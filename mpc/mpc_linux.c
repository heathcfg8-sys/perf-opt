#include "mpc_linux.h"
#include "timestamp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// 车辆参数
#define DT 0.05f
#define LF 2.67f
#define REF_V 10.0f
#define MAX_ITER 50
#define LEARNING_RATE 0.002f
#define GRAD_EPS 1e-2f

#define WEIGHT_CTE 10.0f
#define WEIGHT_EPSI 10.0f
#define WEIGHT_V 5.0f
#define WEIGHT_DELTA 0.1f
#define WEIGHT_A 0.1f
#define WEIGHT_DELTA_DIFF 10.0f
#define WEIGHT_A_DIFF 5.0f
#define MAX_ROLLOUT_LEN 50

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define MAX_DELTA 0.5f
#define MAX_A 1.0f
#define MAX_DELTA_RATE 0.5f
#define MAX_A_RATE 2.0f

#define SIGN(x) ((x) > 0 ? 1.0f : ((x) < 0 ? -1.0f : 0.0f))

#ifndef MPC_VERBOSE
#define MPC_VERBOSE 0
#endif

// 预定义参考路径表，用于加速插值计算
#define REF_TABLE_SIZE 2000
#define REF_X_MIN -20.0f
#define REF_X_MAX 120.0f

static float ref_path_table[REF_TABLE_SIZE];
static float ref_deriv_table[REF_TABLE_SIZE];
static bool ref_table_initialized = false;

// 初始化参考路径表
static void init_ref_table(void) {
    if (ref_table_initialized) return;

#pragma omp parallel for
    for (int i = 0; i < REF_TABLE_SIZE; i++) {
        float x = REF_X_MIN + (REF_X_MAX - REF_X_MIN) * i / (REF_TABLE_SIZE - 1);
        ref_path_table[i] = sinf(0.2f * x) + 0.1f * x;
        ref_deriv_table[i] = 0.2f * cosf(0.2f * x) + 0.1f;
    }
    ref_table_initialized = true;
}

// 获取参考路径值，使用线性插值
static inline float ref_path(float x)
{
    init_ref_table();

    if (x <= REF_X_MIN) return ref_path_table[0];
    if (x >= REF_X_MAX) return ref_path_table[REF_TABLE_SIZE - 1];

    float t = (x - REF_X_MIN) / (REF_X_MAX - REF_X_MIN);
    float idx_float = t * (REF_TABLE_SIZE - 1);
    int idx = (int)idx_float;
    float frac = idx_float - idx;

    return ref_path_table[idx] * (1.0f - frac) + ref_path_table[idx + 1] * frac;
}

static inline float ref_path_derivative(float x)
{
    init_ref_table();

    if (x <= REF_X_MIN) return ref_deriv_table[0];
    if (x >= REF_X_MAX) return ref_deriv_table[REF_TABLE_SIZE - 1];

    float t = (x - REF_X_MIN) / (REF_X_MAX - REF_X_MIN);
    float idx_float = t * (REF_TABLE_SIZE - 1);
    int idx = (int)idx_float;
    float frac = idx_float - idx;

    return ref_deriv_table[idx] * (1.0f - frac) + ref_deriv_table[idx + 1] * frac;
}

// 角度归一化函数
static inline float normalize_angle(float angle)
{
    while (angle > M_PI)
        angle -= 2.0f * M_PI;
    while (angle < -M_PI)
        angle += 2.0f * M_PI;
    return angle;
}

static size_t round_up_to_multiple(size_t value, size_t multiple)
{
    return (value + multiple - 1U) / multiple * multiple;
}

static inline void init_controls(Controls_t* ctrl, int horizon)
{
    size_t bytes = round_up_to_multiple((size_t)horizon * sizeof(float), 16U);

    ctrl->horizon = horizon;
    ctrl->delta = (float*)aligned_alloc(16U, bytes);
    ctrl->a = (float*)aligned_alloc(16U, bytes);
}

static inline void free_controls(Controls_t* ctrl)
{
    if (ctrl->delta)
        free(ctrl->delta);
    if (ctrl->a)
        free(ctrl->a);

    ctrl->delta = NULL;
    ctrl->a = NULL;
    ctrl->horizon = 0;
}

static inline void copy_controls(Controls_t* dest, const Controls_t* src)
{
    dest->horizon = src->horizon;
    memcpy(dest->delta, src->delta, src->horizon * sizeof(float));
    memcpy(dest->a, src->a, src->horizon * sizeof(float));
}

// 执行单步模拟，同时更新方向余弦值
static void simulate(State_t* s, float* cos_psi, float* sin_psi, float delta, float a)
{
    float vx = *cos_psi;
    float vy = *sin_psi;

    float tan_delta = tanf(delta);
    float delta_psi = s->v * tan_delta / LF * DT;

    // 小角度近似优化
    float cos_dpsi, sin_dpsi;
    if (fabsf(delta_psi) < 0.1f) {
        cos_dpsi = 1.0f - delta_psi * delta_psi * 0.5f;
        sin_dpsi = delta_psi;
    }
    else {
        cos_dpsi = cosf(delta_psi);
        sin_dpsi = sinf(delta_psi);
    }

    float new_vx = vx * cos_dpsi - vy * sin_dpsi;
    float new_vy = vy * cos_dpsi + vx * sin_dpsi;

    s->x += s->v * new_vx * DT;
    s->y += s->v * new_vy * DT;

    s->v += a * DT;
    if (s->v < 0.01f)
        s->v = 0.01f;
    else if (s->v > 20.0f)
        s->v = 20.0f;

    *cos_psi = new_vx;
    *sin_psi = new_vy;
    s->psi = atan2f(new_vy, new_vx);
}

// 计算局部代价，包括控制代价和约束惩罚
static float local_cost(const State_t* s,
    const Controls_t* u,
    const Controls_t* prev_u,
    int t)
{
    float ref_x = s->x;
    float f_x = ref_path(ref_x);
    float f_prime = ref_path_derivative(ref_x);
    float desired_psi = atanf(f_prime);

    float cte = s->y - f_x;
    float epsi = normalize_angle(s->psi - desired_psi);
    float v_err = REF_V - s->v;

    float cost = WEIGHT_CTE * cte * cte +
        WEIGHT_EPSI * epsi * epsi +
        WEIGHT_V * v_err * v_err +
        WEIGHT_DELTA * u->delta[t] * u->delta[t] +
        WEIGHT_A * u->a[t] * u->a[t];

    if (prev_u != NULL && t > 0) {
        float delta_diff = u->delta[t] - prev_u->delta[t - 1];
        float a_diff = u->a[t] - prev_u->a[t - 1];

        cost += WEIGHT_DELTA_DIFF * delta_diff * delta_diff;
        cost += WEIGHT_A_DIFF * a_diff * a_diff;

        float delta_rate = fabsf(delta_diff / DT);
        if (delta_rate > MAX_DELTA_RATE) {
            float penalty = delta_rate - MAX_DELTA_RATE;
            cost += 1000.0f * penalty * penalty;
        }
    }

    if (s->v < 0.0f) {
        cost += 5000.0f * s->v * s->v;
    }

    if (fabsf(epsi) > M_PI / 2.0f) {
        float penalty = fabsf(epsi) - (M_PI / 2.0f);
        cost += 1000.0f * penalty * penalty;
    }

    return cost;
}

// 计算尾部代价，使用循环展开优化
static float rollout_tail_cost(State_t start,
    float cos_psi,
    float sin_psi,
    const Controls_t* u_seq,
    int start_t,
    int end_t)
{
    float cost = 0.0f;
    State_t s = start;
    int horizon_len = u_seq->horizon;
    int max_t = (start_t + horizon_len - 1 < end_t) ? (start_t + horizon_len - 1) : end_t;

    // 循环展开模拟4步
    int t = start_t;
    for (; t + 3 <= max_t; t += 4) {
        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t], u_seq->a[t]);
        cost += local_cost(&s, u_seq, u_seq, t);

        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t + 1], u_seq->a[t + 1]);
        cost += local_cost(&s, u_seq, u_seq, t + 1);

        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t + 2], u_seq->a[t + 2]);
        cost += local_cost(&s, u_seq, u_seq, t + 2);

        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t + 3], u_seq->a[t + 3]);
        cost += local_cost(&s, u_seq, u_seq, t + 3);
    }

    for (; t <= max_t; ++t) {
        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t], u_seq->a[t]);
        cost += local_cost(&s, u_seq, u_seq, t);
    }

    return cost;
}

// 缓存版本的梯度计算
static void compute_gradient_cached(State_t current,
    Controls_t* u_seq,
    Controls_t* grad,
    State_t* traj_states,
    float* cos_arr,
    float* sin_arr,
    float* tail_costs)
{
    int horizon = u_seq->horizon;

    // 前向模拟并存储轨迹状态
    traj_states[0] = current;
    cos_arr[0] = cosf(current.psi);
    sin_arr[0] = sinf(current.psi);

    for (int t = 0; t < horizon; t++) {
        traj_states[t + 1] = traj_states[t];
        cos_arr[t + 1] = cos_arr[t];
        sin_arr[t + 1] = sin_arr[t];
        simulate(&traj_states[t + 1], &cos_arr[t + 1], &sin_arr[t + 1],
            u_seq->delta[t], u_seq->a[t]);
    }

    // 并行计算尾部代价
#pragma omp parallel for schedule(static)
    for (int t = 0; t < horizon; t++) {
        tail_costs[t] = rollout_tail_cost(
            traj_states[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);
    }

    // 并行计算梯度
#pragma omp parallel for schedule(static)
    for (int t = 0; t < horizon; t++) {
        float orig_delta = u_seq->delta[t];
        float orig_a = u_seq->a[t];
        float cost_base = tail_costs[t];

        // delta梯度
        u_seq->delta[t] = orig_delta + GRAD_EPS;
        float cost_plus_delta = rollout_tail_cost(
            traj_states[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);

        // a梯度
        u_seq->a[t] = orig_a + GRAD_EPS;
        float cost_plus_a = rollout_tail_cost(
            traj_states[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);

        // 恢复原值
        u_seq->delta[t] = orig_delta;
        u_seq->a[t] = orig_a;

        grad->delta[t] = (cost_plus_delta - cost_base) / GRAD_EPS;
        grad->a[t] = (cost_plus_a - cost_base) / GRAD_EPS;
    }
}

// 原始梯度计算方法，包含临时存储数组
static void compute_gradient(State_t current,
    Controls_t* u_seq,
    Controls_t* grad)
{
    int horizon = u_seq->horizon;
    State_t traj[horizon + 1];
    float cos_arr[horizon + 1];
    float sin_arr[horizon + 1];

    traj[0] = current;
    cos_arr[0] = cosf(current.psi);
    sin_arr[0] = sinf(current.psi);

    for (int t = 0; t < horizon; t++) {
        traj[t + 1] = traj[t];
        cos_arr[t + 1] = cos_arr[t];
        sin_arr[t + 1] = sin_arr[t];
        simulate(&traj[t + 1], &cos_arr[t + 1], &sin_arr[t + 1],
            u_seq->delta[t], u_seq->a[t]);
    }

    float cost_base_tail[horizon];

#pragma omp parallel for
    for (int t = 0; t < horizon; t++) {
        cost_base_tail[t] = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);
    }

#pragma omp parallel for
    for (int t = 0; t < horizon; t++) {
        float orig_delta = u_seq->delta[t];
        float orig_a = u_seq->a[t];

        u_seq->delta[t] = orig_delta + GRAD_EPS;
        float cost_plus_delta = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);
        u_seq->delta[t] = orig_delta;

        u_seq->a[t] = orig_a + GRAD_EPS;
        float cost_plus_a = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t],
            u_seq, t, horizon - 1);
        u_seq->a[t] = orig_a;

        grad->delta[t] = (cost_plus_delta - cost_base_tail[t]) / GRAD_EPS;
        grad->a[t] = (cost_plus_a - cost_base_tail[t]) / GRAD_EPS;
    }
}

// NEON优化的控制更新
static void neon_update_controls(Controls_t* u_seq, const Controls_t* grad)
{
    int horizon = u_seq->horizon;
    int simd_end = horizon - (horizon % 4);

#if defined(__aarch64__) || defined(__ARM_NEON)
    const float32x4_t v_lr = vdupq_n_f32(LEARNING_RATE);
    const float32x4_t v_max_delta = vdupq_n_f32(MAX_DELTA);
    const float32x4_t v_min_delta = vdupq_n_f32(-MAX_DELTA);
    const float32x4_t v_max_a = vdupq_n_f32(MAX_A);
    const float32x4_t v_min_a = vdupq_n_f32(-MAX_A);
    const float32x4_t v_scale_delta = vdupq_n_f32(1.0f / 10000.0f);
    const float32x4_t v_scale_a = vdupq_n_f32(1.0f / 1000.0f);

#pragma omp parallel for
    for (int i = 0; i < simd_end; i += 4) {
        float32x4_t v_delta = vld1q_f32(&u_seq->delta[i]);
        float32x4_t v_grad_d = vld1q_f32(&grad->delta[i]);

        v_grad_d = vmulq_f32(v_grad_d, v_scale_delta);
        v_delta = vmlsq_f32(v_delta, v_lr, v_grad_d);
        v_delta = vmaxq_f32(vminq_f32(v_delta, v_max_delta), v_min_delta);
        vst1q_f32(&u_seq->delta[i], v_delta);

        float32x4_t v_a = vld1q_f32(&u_seq->a[i]);
        float32x4_t v_grad_a = vld1q_f32(&grad->a[i]);

        v_grad_a = vmulq_f32(v_grad_a, v_scale_a);
        v_a = vmlsq_f32(v_a, v_lr, v_grad_a);
        v_a = vmaxq_f32(vminq_f32(v_a, v_max_a), v_min_a);
        vst1q_f32(&u_seq->a[i], v_a);
    }
#else
    simd_end = 0;
#endif

    for (int i = simd_end; i < horizon; ++i) {
        u_seq->delta[i] -= LEARNING_RATE * grad->delta[i] / 10000.0f;
        if (u_seq->delta[i] > MAX_DELTA) {
            u_seq->delta[i] = MAX_DELTA;
        }
        else if (u_seq->delta[i] < -MAX_DELTA) {
            u_seq->delta[i] = -MAX_DELTA;
        }

        u_seq->a[i] -= LEARNING_RATE * grad->a[i] / 1000.0f;
        if (u_seq->a[i] > MAX_A) {
            u_seq->a[i] = MAX_A;
        }
        else if (u_seq->a[i] < -MAX_A) {
            u_seq->a[i] = -MAX_A;
        }
    }
}

// 初始化线程缓存
void mpc_cache_init(MPCThreadCache_t* cache, int max_horizon) {
    if (cache->initialized && cache->max_horizon >= max_horizon) {
        return;
    }

    if (cache->initialized) {
        mpc_cache_free(cache);
    }

    cache->max_horizon = max_horizon;
    init_controls(&cache->u_seq, max_horizon);
    init_controls(&cache->grad, max_horizon);

    cache->traj_states = (State_t*)aligned_alloc(16U, (max_horizon + 1) * sizeof(State_t));
    cache->cos_arr = (float*)aligned_alloc(16U, (max_horizon + 1) * sizeof(float));
    cache->sin_arr = (float*)aligned_alloc(16U, (max_horizon + 1) * sizeof(float));
    cache->tail_costs = (float*)aligned_alloc(16U, max_horizon * sizeof(float));
    cache->initialized = true;
}

void mpc_cache_free(MPCThreadCache_t* cache) {
    if (!cache->initialized) return;

    free_controls(&cache->u_seq);
    free_controls(&cache->grad);
    free(cache->traj_states);
    free(cache->cos_arr);
    free(cache->sin_arr);
    free(cache->tail_costs);
    cache->initialized = false;
}

// 带缓存的MPC优化主函数
static void mpc_control_with_plan_cached(State_t current, Controls_t* plan_out, MPCThreadCache_t* cache)
{
    int horizon = plan_out->horizon;

    // 确保缓存足够大
    if (!cache->initialized || cache->max_horizon < horizon) {
        mpc_cache_init(cache, horizon);
    }

    // 如果没有现有目标，写入缓存大小
    cache->u_seq.horizon = horizon;
    cache->grad.horizon = horizon;

    // 初始化解为零
#pragma omp parallel for
    for (int i = 0; i < horizon; ++i) {
        cache->u_seq.delta[i] = 0.0f;
        cache->u_seq.a[i] = 0.0f;
    }

    // 梯度下降优化
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        compute_gradient_cached(current, &cache->u_seq, &cache->grad,
            cache->traj_states, cache->cos_arr,
            cache->sin_arr, cache->tail_costs);
        neon_update_controls(&cache->u_seq, &cache->grad);

        // 添加速率限制
        for (int t = 1; t < horizon; ++t) {
            float delta_rate = cache->u_seq.delta[t] - cache->u_seq.delta[t - 1];
            if (fabsf(delta_rate) > MAX_DELTA_RATE * DT) {
                cache->u_seq.delta[t] = cache->u_seq.delta[t - 1] + SIGN(delta_rate) * MAX_DELTA_RATE * DT;
            }

            float a_rate = cache->u_seq.a[t] - cache->u_seq.a[t - 1];
            if (fabsf(a_rate) > MAX_A_RATE * DT) {
                cache->u_seq.a[t] = cache->u_seq.a[t - 1] + SIGN(a_rate) * MAX_A_RATE * DT;
            }
        }
    }

#pragma omp parallel for
    for (int i = 0; i < horizon; ++i) {
        plan_out->delta[i] = cache->u_seq.delta[i];
        plan_out->a[i] = cache->u_seq.a[i];
    }
}

// 原始MPC优化函数（无缓存）
static void mpc_control_with_plan(State_t current, Controls_t* plan_out)
{
    Controls_t u_seq;
    Controls_t grad;

    u_seq.horizon = plan_out->horizon;
    grad.horizon = plan_out->horizon;

    init_controls(&u_seq, u_seq.horizon);
    init_controls(&grad, grad.horizon);

    int horizon = plan_out->horizon;

#pragma omp parallel for
    for (int i = 0; i < horizon; ++i) {
        u_seq.delta[i] = 0.0f;
        u_seq.a[i] = 0.0f;
    }

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        compute_gradient(current, &u_seq, &grad);
        neon_update_controls(&u_seq, &grad);

        for (int t = 1; t < horizon; ++t) {
            float delta_rate = u_seq.delta[t] - u_seq.delta[t - 1];
            if (fabsf(delta_rate) > MAX_DELTA_RATE * DT) {
                u_seq.delta[t] = u_seq.delta[t - 1] + SIGN(delta_rate) * MAX_DELTA_RATE * DT;
            }

            float a_rate = u_seq.a[t] - u_seq.a[t - 1];
            if (fabsf(a_rate) > MAX_A_RATE * DT) {
                u_seq.a[t] = u_seq.a[t - 1] + SIGN(a_rate) * MAX_A_RATE * DT;
            }
        }
    }

#pragma omp parallel for
    for (int i = 0; i < horizon; ++i) {
        plan_out->delta[i] = u_seq.delta[i];
        plan_out->a[i] = u_seq.a[i];
    }

    free_controls(&u_seq);
    free_controls(&grad);
}

static void print_predicted_trajectory(State_t s, const Controls_t* plan_out)
{
    int horizon = plan_out->horizon;
    State_t state = s;
    float cos_psi = cosf(state.psi);
    float sin_psi = sinf(state.psi);
    float cte_avg = 0.0f;

    for (int t = 0; t < horizon; ++t) {
        simulate(&state, &cos_psi, &sin_psi, plan_out->delta[t], plan_out->a[t]);

        float ref_y = ref_path(state.x);
        float cte = state.y - ref_y;
        cte_avg += fabsf(cte);
    }

    printf("Average CTE over horizon: %.3f\n", cte_avg / (float)horizon);
}

// 带缓存的外部接口
RetCode_t mpc_linux_iopointer_cached(int horizon, void* input, void* output, MPCThreadCache_t* cache)
{
    if (horizon <= 0)
        return K_RET_INVALID_PARAM;

    if (input == NULL || output == NULL || cache == NULL)
        return K_RET_INVALID_PARAM;

    State_t* in = (State_t*)input;
    Controls_t* out_ctrl = (Controls_t*)output;

#if MPC_VERBOSE
    printf("Initial State: x=%.3f, y=%.3f, psi=%.3f, v=%.3f, cte=%.3f, epsi=%.3f\n",
        in->x, in->y, in->psi, in->v, in->cte, in->epsi);
#endif

    init_controls(out_ctrl, horizon);
    mpc_control_with_plan_cached(*in, out_ctrl, cache);

#if MPC_VERBOSE
    print_predicted_trajectory(*in, out_ctrl);

    for (int i = 0; i < horizon; ++i) {
        printf("Control %d: delta=%.3f, a=%.3f\n",
            i, out_ctrl->delta[i], out_ctrl->a[i]);
    }
#endif

    return K_RET_OK;
}

// 原始外部接口（无缓存）
RetCode_t mpc_linux_iopointer(int horizon, void* input, void* output)
{
    if (horizon <= 0)
        return K_RET_INVALID_PARAM;

    if (input == NULL || output == NULL)
        return K_RET_INVALID_PARAM;

    State_t* in = (State_t*)input;
    Controls_t* out_ctrl = (Controls_t*)output;

    // 直接输出，不使用条件编译
    printf("Initial State: x=%.3f, y=%.3f, psi=%.3f, v=%.3f, cte=%.3f, epsi=%.3f\n",
        in->x, in->y, in->psi, in->v, in->cte, in->epsi);

    init_controls(out_ctrl, horizon);
    mpc_control_with_plan(*in, out_ctrl);

    // 直接输出，不使用条件编译
    print_predicted_trajectory(*in, out_ctrl);

    for (int i = 0; i < horizon; ++i) {
        printf("Control %d: delta=%.3f, a=%.3f\n",
            i, out_ctrl->delta[i], out_ctrl->a[i]);
    }

    return K_RET_OK;
}

RetCode_t mpc_linux_ioself_profiling(int horizon)
{
    if (horizon <= 0)
        return K_RET_INVALID_PARAM;

    State_t* in = (State_t*)malloc(sizeof(State_t));
    Controls_t* out = (Controls_t*)malloc(sizeof(Controls_t));

    if (in == NULL || out == NULL) {
        free(in);
        free(out);
        return K_RET_UNK_ERROR;
    }

    out->horizon = 0;
    out->delta = NULL;
    out->a = NULL;

    // 随机生成输入状态
    srand(time(NULL));

    in->x = (float)(rand() % 20);
    in->y = ref_path(in->x) + ((float)(rand() % 5) - 2);
    in->psi = atanf(ref_path_derivative(in->x)) + ((float)(rand() % 21 - 10) / 100.0f);
    in->v = REF_V * (0.1f + 0.4f * (float)(rand() % 100) / 100.0f);
    in->cte = in->y - ref_path(in->x);
    in->epsi = normalize_angle(in->psi - atanf(ref_path_derivative(in->x)));

    // 直接在这里添加输出
    printf("Initial State: x=%.3f, y=%.3f, psi=%.3f, v=%.3f, cte=%.3f, epsi=%.3f\n",
        in->x, in->y, in->psi, in->v, in->cte, in->epsi);

    struct timespec start_time = timestamp();

    RetCode_t ret = mpc_linux_iopointer(horizon, (void*)in, (void*)out);

    struct timespec end_time = timestamp();
    int64_t total_time_ns = timestamp_diff(start_time, end_time);
    double total_time_ms = (double)total_time_ns / 1000000.0;

    // 在这里添加控制输出
    printf("Average CTE over horizon: need to compute\n");
    for (int i = 0; i < horizon && i < 39; ++i) {  // 限制输出数量
        if (out->delta != NULL && out->a != NULL) {
            printf("Control %d: delta=%.3f, a=%.3f\n", 
                   i, out->delta[i], out->a[i]);
        }
    }

    if (ret != K_RET_OK) {
        printf("mpc_linux_ioself_profiling: inner call failed with %d\n", ret);
    }
    else {
        printf("mpc_linux_ioself_profiling: horizon=%d total_time=%lld ns %.6f ms\n",
            horizon,
            (long long)total_time_ns,
            total_time_ms);
    }

    free_controls(out);
    free(in);
    free(out);

    return ret;
}

// 带缓存的性能测试函数
RetCode_t mpc_linux_ioself_profiling_cached(int horizon)
{
    if (horizon <= 0)
        return K_RET_INVALID_PARAM;

    State_t* in = (State_t*)malloc(sizeof(State_t));
    Controls_t* out = (Controls_t*)malloc(sizeof(Controls_t));
    MPCThreadCache_t cache = { 0 };

    if (in == NULL || out == NULL) {
        free(in);
        free(out);
        return K_RET_UNK_ERROR;
    }

    out->horizon = 0;
    out->delta = NULL;
    out->a = NULL;

    srand(time(NULL));

    in->x = (float)(rand() % 20);
    in->y = ref_path(in->x) + ((float)(rand() % 5) - 2);
    in->psi = atanf(ref_path_derivative(in->x)) + ((float)(rand() % 21 - 10) / 100.0f);
    in->v = REF_V * (0.1f + 0.4f * (float)(rand() % 100) / 100.0f);
    in->cte = in->y - ref_path(in->x);
    in->epsi = normalize_angle(in->psi - atanf(ref_path_derivative(in->x)));

    struct timespec start_time = timestamp();

    RetCode_t ret = mpc_linux_iopointer_cached(horizon, (void*)in, (void*)out, &cache);

    struct timespec end_time = timestamp();
    int64_t total_time_ns = timestamp_diff(start_time, end_time);
    double total_time_ms = (double)total_time_ns / 1000000.0;

    if (ret != K_RET_OK) {
        printf("mpc_linux_ioself_profiling_cached: inner call failed with %d\n", ret);
    }
    else {
        printf("mpc_linux_ioself_profiling_cached: horizon=%d total_time=%lld ns %.6f ms\n",
            horizon,
            (long long)total_time_ns,
            total_time_ms);
    }

    free_controls(out);
    free(in);
    free(out);
    mpc_cache_free(&cache);

    return ret;
}

RetCode_t mpc_linux_ioself(int horizon)
{
    if (horizon <= 0)
        return K_RET_INVALID_PARAM;

    State_t* in = (State_t*)malloc(sizeof(State_t));
    Controls_t* out = (Controls_t*)malloc(sizeof(Controls_t));

    if (in == NULL || out == NULL) {
        free(in);
        free(out);
        return K_RET_UNK_ERROR;
    }

    out->horizon = 0;
    out->delta = NULL;
    out->a = NULL;

    in->x = 0.0f;
    in->y = 0.0f;
    in->psi = atanf(ref_path_derivative(0.0f));
    in->v = 5.0f;
    in->cte = ref_path(in->x) - in->y;
    in->epsi = in->psi - atanf(ref_path_derivative(in->x));

    RetCode_t ret = mpc_linux_iopointer(horizon, (void*)in, (void*)out);

    free_controls(out);
    free(in);
    free(out);

    return ret;
}