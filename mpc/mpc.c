#include "mpc.h"

// 参考路径函数实现
static inline float ref_path(float x) {
    return sinf(0.2f * x) + 0.1f * x;
}

static inline float ref_path_derivative(float x) {
    return 0.2f * cosf(0.2f * x) + 0.1f;
}

// 角度归一化函数实现
static inline float normalize_angle(float angle) {
    while (angle > M_PI)
        angle -= 2.0f * M_PI;
    while (angle < -M_PI)
        angle += 2.0f * M_PI;
    return angle;
}

// 内存管理函数实现
static inline void init_controls(Controls_t *ctrl, int horizon) {
    ctrl->horizon = horizon;
    ctrl->delta = (float*)aligned_alloc(16, horizon * sizeof(float));
    ctrl->a = (float*)aligned_alloc(16, horizon * sizeof(float));
}

static inline void free_controls(Controls_t *ctrl) {
    if (ctrl->delta) free(ctrl->delta);
    if (ctrl->a) free(ctrl->a);
    ctrl->delta = NULL;
    ctrl->a = NULL;
    ctrl->horizon = 0;
}

static inline void copy_controls(Controls_t *dest, const Controls_t *src) {
    dest->horizon = src->horizon;
    memcpy(dest->delta, src->delta, src->horizon * sizeof(float));
    memcpy(dest->a, src->a, src->horizon * sizeof(float));
}

// 核心MPC函数实现
void simulate(State_t *s, float *cos_psi, float *sin_psi, float delta, float a) {
    float vx = *cos_psi;
    float vy = *sin_psi;

    float delta_psi = s->v * tanf(delta) / LF * DT;
    float cos_dpsi = cosf(delta_psi);
    float sin_dpsi = sinf(delta_psi);

    float new_vx = vx * cos_dpsi - vy * sin_dpsi;
    float new_vy = vy * cos_dpsi + vx * sin_dpsi;

    s->x += s->v * new_vx * DT;
    s->y += s->v * new_vy * DT;

    // 推进速度
    s->v += a * DT;
    if (s->v < 0.01f) s->v = 0.01f;
    else if (s->v > 20.0f) s->v = 20.0f;

    // 更新方向角与 cos/sin
    *cos_psi = new_vx;
    *sin_psi = new_vy;
    s->psi = atan2f(new_vy, new_vx);
}

float local_cost(const State_t *s,
                const Controls_t *u,
                const Controls_t *prev_u,
                int t) {
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
        if (delta_rate > MAX_DELTA_RATE){
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

float rollout_tail_cost(State_t start,
                       float cos_psi,
                       float sin_psi,
                       const Controls_t *u_seq,
                       int start_t, int end_t) {
    float cost = 0.0f;
    State_t s = start;
    int horizon_len = u_seq->horizon;
    int max_t = (start_t + horizon_len - 1 < end_t) ? (start_t + horizon_len - 1) : end_t;

    for (int t = start_t; t <= max_t; ++t) {
        simulate(&s, &cos_psi, &sin_psi, u_seq->delta[t], u_seq->a[t]);
        cost += local_cost(&s, u_seq, u_seq, t);
    }
    return cost;
}

void compute_gradient(State_t current,
                     Controls_t *u_seq,
                     Controls_t *grad) 
{
    int horizon = u_seq->horizon;
    State_t traj[horizon+1];
    float cos_arr[horizon+1];
    float sin_arr[horizon+1];
    
    traj[0] = current;
    cos_arr[0] = cosf(current.psi);
    sin_arr[0] = sinf(current.psi);
    
    for (int t = 0; t < horizon; t++) {
        traj[t+1] = traj[t];
        cos_arr[t+1] = cos_arr[t];
        sin_arr[t+1] = sin_arr[t];
        simulate(&traj[t+1], &cos_arr[t+1], &sin_arr[t+1], 
                 u_seq->delta[t], u_seq->a[t]);
    }

    float cost_base_tail[horizon];
    #pragma omp parallel for
    for (int t = 0; t < horizon; t++) {
        cost_base_tail[t] = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t], 
            u_seq, t, horizon-1
        );
    }

    #pragma omp parallel for
    for (int t = 0; t < horizon; t++) {

        float orig_delta = u_seq->delta[t];
        float orig_a = u_seq->a[t];
        
        // delta梯度
        u_seq->delta[t] = orig_delta + GRAD_EPS;
        float cost_plus_delta = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t], 
            u_seq, t, horizon-1
        );
        u_seq->delta[t] = orig_delta;
        
        // a梯度
        u_seq->a[t] = orig_a + GRAD_EPS;
        float cost_plus_a = rollout_tail_cost(
            traj[t], cos_arr[t], sin_arr[t], 
            u_seq, t, horizon-1
        );
        u_seq->a[t] = orig_a;
        
        // 有限差分
        grad->delta[t] = (cost_plus_delta - cost_base_tail[t]) / GRAD_EPS;
        grad->a[t] = (cost_plus_a - cost_base_tail[t]) / GRAD_EPS;
    }
}

void neon_update_controls(Controls_t* u_seq, const Controls_t* grad) {
    int horizon = u_seq->horizon;
    const float32x4_t v_lr = vdupq_n_f32(LEARNING_RATE);
    const float32x4_t v_max_delta = vdupq_n_f32(MAX_DELTA);
    const float32x4_t v_min_delta = vdupq_n_f32(-MAX_DELTA);
    const float32x4_t v_max_a = vdupq_n_f32(MAX_A);
    const float32x4_t v_min_a = vdupq_n_f32(-MAX_A);

    #pragma omp parallel for simd
    for (int i = 0; i < horizon; i += 4) {
        // Delta处理
        float32x4_t v_delta = vld1q_f32(&u_seq->delta[i]);
        float32x4_t v_grad_d = vld1q_f32(&grad->delta[i]);
        v_delta = vmlsq_f32(v_delta, v_lr, v_grad_d / 10000); 
        v_delta = vmaxq_f32(vminq_f32(v_delta, v_max_delta), v_min_delta);
        vst1q_f32(&u_seq->delta[i], v_delta);

        // A处理
        float32x4_t v_a = vld1q_f32(&u_seq->a[i]);
        float32x4_t v_grad_a = vld1q_f32(&grad->a[i]);
        v_a = vmlsq_f32(v_a, v_lr, v_grad_a / 1000); 
        v_a = vmaxq_f32(vminq_f32(v_a, v_max_a), v_min_a);
        vst1q_f32(&u_seq->a[i], v_a);
    }
}

void print_predicted_trajectory(State_t s, const Controls_t* plan_out) {
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

void mpc_control_with_plan(State_t current, Controls_t *plan_out) {
    Controls_t u_seq;
    Controls_t grad;
    u_seq.horizon = plan_out->horizon;
    grad.horizon = plan_out->horizon;
    init_controls(&u_seq, u_seq.horizon);
    init_controls(&grad, grad.horizon);
    int horizon = plan_out->horizon;
    
    // 初始化控制序列
    #pragma omp parallel for
    for (int i = 0; i < horizon; ++i) { 
        u_seq.delta[i] = 0.0f;
        u_seq.a[i] = 0.0f;
    }
    
    #pragma omp parallel for
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        compute_gradient(current, &u_seq, &grad);
        neon_update_controls(&u_seq, &grad);
        // 控制变化率约束
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

// Linux接口函数实现
int mpc_linux_iopointer(int horizon, const void *input, void *output) {
    if (horizon <= 0) return -1;
    if (input == NULL || output == NULL) return -2;

    // 强制类型转换
    const State_t *in = (const State_t *)input;
    Controls_t *out_ctrl = (Controls_t *)output;

    printf("Initial State: x=%.3f, y=%.3f, psi=%.3f, v=%.3f, cte=%.3f, epsi=%.3f\n",
           in->x, in->y, in->psi, in->v, in->cte, in->epsi);

    init_controls(out_ctrl, horizon);
    mpc_control_with_plan(*in, out_ctrl);
    print_predicted_trajectory(*in, out_ctrl);

    for (int i = 0; i < horizon; ++i) {
        printf("Control %d: delta=%.3f, a=%.3f\n", i, out_ctrl->delta[i], out_ctrl->a[i]);
    }

    return 0;
}

int mpc_linux_ioself_profiling(int horizon) {
    if (horizon <= 0) return -1;
    
    State_t *in = (State_t *)malloc(sizeof(State_t));
    Controls_t *out = (Controls_t *)malloc(sizeof(Controls_t));
    if (in == NULL || out == NULL) {
        free(in); free(out);
        return -2;
    }

    init_controls(out, horizon);

    // 随机生成输入状态
    srand(time(NULL));
    in->x = (float)(rand() % 20);
    in->y = ref_path(in->x) + ((float)(rand() % 5) - 2);
    in->psi = atanf(ref_path_derivative(in->x)) + ((float)(rand() % 21 - 10) / 100.0f);
    in->v = REF_V * (0.1f + 0.4f * (float)(rand() % 100) / 100.0f);
    in->cte = in->y - ref_path(in->x);
    in->epsi = normalize_angle(in->psi - atanf(ref_path_derivative(in->x)));

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int ret = mpc_linux_iopointer(horizon, (const void *)in, (void *)out);

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double diff_sec = (double)(end_time.tv_sec - start_time.tv_sec);
    double diff_nsec = (double)(end_time.tv_nsec - start_time.tv_nsec);
    if (diff_nsec < 0) {
        diff_sec -= 1.0;
        diff_nsec += 1e9;
    }
    double total_time_ns = diff_sec * 1e9 + diff_nsec;
    double total_time_ms = total_time_ns / 1e6;

    if (ret != 0) {
        printf("mpc_linux_ioself_profiling: inner call failed with %d\n", ret);
    } else {
        printf("mpc_linux_ioself_profiling: horizon=%d total_time=%.6f ms\n",
               horizon, total_time_ms);
    }

    free_controls(out);
    free(in);
    free(out);
    return ret;
}

int mpc_linux_ioself(int horizon) {
    if (horizon <= 0) return -1;

    State_t *in = (State_t *)malloc(sizeof(State_t));
    Controls_t *out = (Controls_t *)malloc(sizeof(Controls_t));
    if (in == NULL || out == NULL) {
        free(in); free(out);
        return -2;
    }

    init_controls(out, horizon);

    // 固定起点测试
    in->x = 0.0f;
    in->y = 0.0f;
    in->psi = atanf(ref_path_derivative(0.0f)); 
    in->v = 5.0f;
    in->cte = ref_path(in->x) - in->y;
    in->epsi = in->psi - atanf(ref_path_derivative(in->x));

    int ret = mpc_linux_iopointer(horizon, (const void *)in, (void *)out);

    free_controls(out);
    free(in);
    free(out);
    return ret;
}