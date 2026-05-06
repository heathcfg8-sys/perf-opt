

#include "fft_a.h"
#define PI 3.14159265358979323846

float **array_r = NULL;
float **array_i = NULL;

Complex complex_add(Complex a, Complex b) {
    Complex r = {a.real + b.real, a.imag + b.imag};
    return r;
}

Complex complex_sub(Complex a, Complex b) {
    Complex r = {a.real - b.real, a.imag - b.imag};
    return r;
}

Complex complex_mul(Complex a, Complex b) {
    Complex r = {
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    };
    return r;
}

// ---------------- 预处理旋转因子（原有代码保留，修改为动态规模） ----------------
void preCompute(int size) {

    int max_level = (int)(log2(size) + 1);
    array_r = (float**)malloc(sizeof(float*) * max_level);
    array_i = (float**)malloc(sizeof(float*) * max_level);
    
    int level = 0;
    for (int le = 2; le <= size; le <<= 1, level++) {
        int le2 = le / 2;
        int step = size / le;
        array_r[level] = (float*)malloc(sizeof(float) * le2);
        array_i[level] = (float*)malloc(sizeof(float) * le2);
        for (int k = 0; k < le2; k++) {
            int idx = k * step;
            double angle = -2 * PI * idx / size;
            array_r[level][k] = (float)cos(angle);
            array_i[level][k] = (float)sin(angle);
        }
    }
}

// ---------------- FFT（原有代码保留，修改为动态规模） ----------------
void fft(float *x_real, float *x_imag, int size) {
    int n = size;
    int i, j, k, m;
    int le, le2;
    float ur, ui;
    Complex u, t;
    int level = 0;

    // 位反转重排
    j = 0;
    for (i = 1; i < n; i++) {
        m = n >> 1;
        while (j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
        if (i < j) {
            float tmp_r = x_real[i];
            float tmp_i = x_imag[i];
            x_real[i] = x_real[j];
            x_imag[i] = x_imag[j];
            x_real[j] = tmp_r;
            x_imag[j] = tmp_i;
        }
    }

    // FFT主体（NEON优化保留）
    for (le = 2; le <= n; le <<= 1, level++) {
        le2 = le / 2;
        for (j = 0; j < n; j += le) {
            if (le <= 4) {
                for (k = 0; k < le2; k++) {
                    int ip = j + k + le2;
                    ur = array_r[level][k];
                    ui = array_i[level][k];
                    t = complex_mul((Complex){ur, ui}, (Complex){x_real[ip], x_imag[ip]});
                    u.real = x_real[j + k];
                    u.imag = x_imag[j + k];
                    Complex r1 = complex_add(u, t);
                    Complex r2 = complex_sub(u, t);
                    x_real[j + k] = r1.real;
                    x_imag[j + k] = r1.imag;
                    x_real[ip] = r2.real;
                    x_imag[ip] = r2.imag;
                }
            } else {
                for (k = 0; k < le2; k += 4) {
                    int idx_base = k * (n / le);
                    // 旋转因子表已改为 float 连续数组，可直接加载，避免 double 到 float 的临时拷贝
                    float32x4_t wr_v = vld1q_f32(&array_r[level][k]);
                    float32x4_t wi_v = vld1q_f32(&array_i[level][k]);
                    float32x4_t xr_ip = vld1q_f32(&x_real[j + k + le2]);
                    float32x4_t xi_ip = vld1q_f32(&x_imag[j + k + le2]);

                    float32x4_t t_real = vmlsq_f32(vmulq_f32(wr_v, xr_ip), wi_v, xi_ip);
                    float32x4_t t_imag = vmlaq_f32(vmulq_f32(wr_v, xi_ip), wi_v, xr_ip);

                    float32x4_t xr_u = vld1q_f32(&x_real[j + k]);
                    float32x4_t xi_u = vld1q_f32(&x_imag[j + k]);

                    float32x4_t xr_sum = vaddq_f32(xr_u, t_real);
                    float32x4_t xi_sum = vaddq_f32(xi_u, t_imag);
                    float32x4_t xr_diff = vsubq_f32(xr_u, t_real);
                    float32x4_t xi_diff = vsubq_f32(xi_u, t_imag);

                    vst1q_f32(&x_real[j + k], xr_sum);
                    vst1q_f32(&x_imag[j + k], xi_sum);
                    vst1q_f32(&x_real[j + k + le2], xr_diff);
                    vst1q_f32(&x_imag[j + k + le2], xi_diff);
                }
            }
        }
    }
}

void ifft(float *x_real, float *x_imag, int size) {
    for (int i = 0; i < size; i++) {
        x_imag[i] = -x_imag[i];
    }
    fft(x_real, x_imag, size);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

// ---------------- 1. 核心函数：fft_linux_iopointer（用户提供输入输出指针） ----------------
int fft_linux_iopointer(int size, float *input_real, float *input_imag) {
    // 检查输入合法性
    if (input_real == NULL || input_imag == NULL) {
        printf("Error: Input pointer is NULL\n");
        return -1;
    }
    if ((size & (size - 1)) != 0) { // 仅支持2的幂次规模
        printf("Error: Size must be power of 2\n");
        return -2;
    }

    // 预处理旋转因子（按当前规模）
    preCompute(size);

    // 执行FFT（核心MPC算子逻辑）
    fft(input_real, input_imag, size);

    return 0; // 无错误返回0
}

// ---------------- 2. 函数：fft_linux_ioself_profiling（自生成数据+计时） ----------------
int fft_linux_ioself_profiling(int size) {
    // 检查规模合法性
    if ((size & (size - 1)) != 0) {
        printf("Error: Size %d is not power of 2\n", size);
        return -1;
    }

    // 1. 自生成输入数据（随机浮点数，模拟PCM信号）
    float *input_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *input_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    if (input_real == NULL || input_imag == NULL || output_real == NULL || output_imag == NULL) {
        printf("Error: Allocate memory failed for size %d\n", size);
        // 释放已分配内存
        if (input_real != NULL) free(input_real);
        if (input_imag != NULL) free(input_imag);
        if (output_real != NULL) free(output_real);
        if (output_imag != NULL) free(output_imag);
        return -2;
    }

    // 初始化输入：实部为随机值（-32768~32767），虚部为0（模拟PCM）
    srand((unsigned int)time(NULL));
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);
    // 2. 计时并调用核心函数
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int ret = fft_linux_iopointer(size,output_real, output_imag);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 3. 计算并打印时间（保留4位小数，单位ms）
    double seconds = (end.tv_sec - start.tv_sec);
    double nanoseconds = (end.tv_nsec - start.tv_nsec) / 1e6;
    if (end.tv_nsec < start.tv_nsec) {
        seconds -= 1;
        nanoseconds += 1000;
    }
    double elapsed = 1000*seconds + nanoseconds;

    ifft(output_real, output_imag, size);

    // 计算最大误差（验证正确性）
    double max_diff = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }

    // 打印结果（规模、时间、误差）
    printf("Size: %6d | FFT Time: %.4f ms | Max Error: %.6f\n", size, elapsed, max_diff);

    // 5. 释放所有内存
    free(input_real);
    free(input_imag);
    free(output_real);
    free(output_imag);
    return ret;
}

// ---------------- 3. 函数：fft_linux_ioself（自生成数据+不计时） ----------------
int fft_linux_ioself(int size) {
    // 检查规模合法性
    if ((size & (size - 1)) != 0) {
        printf("Error: Size %d is not power of 2\n", size);
        return -1;
    }

    // 1. 自生成输入数据（同profiling版本）
    float *input_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *input_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    if (input_real == NULL || input_imag == NULL || output_real == NULL || output_imag == NULL) {
        printf("Error: Allocate memory failed for size %d\n", size);
        if (input_real != NULL) free(input_real);
        if (input_imag != NULL) free(input_imag);
        if (output_real != NULL) free(output_real);
        if (output_imag != NULL) free(output_imag);
        return -2;
    }

    srand((unsigned int)time(NULL));
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);
    // 2. 调用核心函数（不计时、不打印）
    int ret = fft_linux_iopointer(size, output_real, output_imag);

    // 3. 释放内存（输出数据已生成，用户可按需修改为返回或存储）
    free(input_real);
    free(input_imag);
    free(output_real);
    free(output_imag);
    return ret;
}

