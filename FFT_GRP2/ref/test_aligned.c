/**
 * FFT 内存对齐 + NEON 优化版本
 * 
 * 优化策略：
 * 1. 16字节内存对齐 - 优化NEON加载效率
 * 2. 预取指令 - 提前加载数据到缓存
 * 3. 寄存器临时变量 - 减少内存访问
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <arm_neon.h>

#define PI 3.14159265358979323846f

/* 位反转变量 */
static void bit_reverse(float *x_real, float *x_imag, int size) {
    int j = 0;
    for (int i = 0; i < size; i++) {
        if (i < j) {
            float tmp_r = x_real[i];
            float tmp_i = x_imag[i];
            x_real[i] = x_real[j];
            x_imag[i] = x_imag[j];
            x_real[j] = tmp_r;
            x_imag[j] = tmp_i;
        }
        int m = size >> 1;
        while (m > 0 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

/* FFT 内存对齐 + NEON 版本 */
static void fft_aligned_neon(float *x_real, float *x_imag, int size,
                             double **twiddle_r, double **twiddle_i) {
    bit_reverse(x_real, x_imag, size);
    
    int level = 0;
    for (int le = 2; le <= size; le <<= 1, level++) {
        int le2 = le / 2;
        
        for (int j = 0; j < size; j += le) {
            if (le <= 4) {
                for (int k = 0; k < le2; k++) {
                    int ip = j + k + le2;
                    float wr = (float)twiddle_r[level][k];
                    float wi = (float)twiddle_i[level][k];
                    
                    float xr_ip = x_real[ip];
                    float xi_ip = x_imag[ip];
                    float tr = wr * xr_ip - wi * xi_ip;
                    float ti = wr * xi_ip + wi * xr_ip;
                    
                    float xr_u = x_real[j + k];
                    float xi_u = x_imag[j + k];
                    
                    x_real[j + k] = xr_u + tr;
                    x_imag[j + k] = xi_u + ti;
                    x_real[ip] = xr_u - tr;
                    x_imag[ip] = xi_u - ti;
                }
            } else {
                /* NEON向量化 + 预取 */
                for (int k = 0; k < le2; k += 4) {
                    /* 预取下一批数据 */
                    if (k + 8 < le2) {
                        __builtin_prefetch(&x_real[j + k + le2 + 8], 0, 1);
                        __builtin_prefetch(&x_imag[j + k + le2 + 8], 0, 1);
                    }
                    
                    float wr[4], wi[4];
                    for (int t = 0; t < 4 && k + t < le2; t++) {
                        wr[t] = (float)twiddle_r[level][k + t];
                        wi[t] = (float)twiddle_i[level][k + t];
                    }
                    
                    /* NEON加载和处理 */
                    float32x4_t wr_v = vld1q_f32(wr);
                    float32x4_t wi_v = vld1q_f32(wi);
                    
                    /* 使用临时变量减少重复加载 */
                    float *xr_ip_ptr = &x_real[j + k + le2];
                    float *xi_ip_ptr = &x_imag[j + k + le2];
                    float32x4_t xr_ip = vld1q_f32(xr_ip_ptr);
                    float32x4_t xi_ip = vld1q_f32(xi_ip_ptr);
                    
                    /* 融合乘加运算 */
                    float32x4_t tr_v = vmlsq_f32(vmulq_f32(wr_v, xr_ip), wi_v, xi_ip);
                    float32x4_t ti_v = vmlaq_f32(vmulq_f32(wr_v, xi_ip), wi_v, xr_ip);
                    
                    float32x4_t xr_u = vld1q_f32(&x_real[j + k]);
                    float32x4_t xi_u = vld1q_f32(&x_imag[j + k]);
                    
                    float32x4_t xr_sum = vaddq_f32(xr_u, tr_v);
                    float32x4_t xi_sum = vaddq_f32(xi_u, ti_v);
                    float32x4_t xr_diff = vsubq_f32(xr_u, tr_v);
                    float32x4_t xi_diff = vsubq_f32(xi_u, ti_v);
                    
                    vst1q_f32(&x_real[j + k], xr_sum);
                    vst1q_f32(&x_imag[j + k], xi_sum);
                    vst1q_f32(xr_ip_ptr, xr_diff);
                    vst1q_f32(xi_ip_ptr, xi_diff);
                }
            }
        }
    }
}

static void ifft_aligned_neon(float *x_real, float *x_imag, int size,
                              double **twiddle_r, double **twiddle_i) {
    for (int i = 0; i < size; i++) x_imag[i] = -x_imag[i];
    fft_aligned_neon(x_real, x_imag, size, twiddle_r, twiddle_i);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

int main() {
    int sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
    int iterations = 100;
    
    printf("\n");
    printf("================================================================================\n");
    printf("    FFT Optimization Benchmark: Memory Alignment + NEON + Prefetch\n");
    printf("================================================================================\n");
    printf("+----------+------------+---------+\n");
    printf("|   Size   |   Time(ms) |  Error  |\n");
    printf("+----------+------------+---------+\n");
    
    for (int s = 0; s < 8; s++) {
        int size = sizes[s];
        
        int max_level = 0;
        for (int temp = 2; temp <= size; temp <<= 1) max_level++;
        
        double **twiddle_r = (double**)malloc(sizeof(double*) * max_level);
        double **twiddle_i = (double**)malloc(sizeof(double*) * max_level);
        
        int level = 0;
        for (int le = 2; le <= size; le <<= 1, level++) {
            int le2 = le / 2;
            int step = size / le;
            twiddle_r[level] = (double*)malloc(sizeof(double) * le2);
            twiddle_i[level] = (double*)malloc(sizeof(double) * le2);
            for (int k = 0; k < le2; k++) {
                int idx = k * step;
                double angle = -2 * PI * idx / size;
                twiddle_r[level][k] = cos(angle);
                twiddle_i[level][k] = sin(angle);
            }
        }
        
        /* 16字节对齐的内存分配 */
        float *input_real, *input_imag, *output_real, *output_imag;
        posix_memalign((void**)&input_real, 16, sizeof(float) * size);
        posix_memalign((void**)&input_imag, 16, sizeof(float) * size);
        posix_memalign((void**)&output_real, 16, sizeof(float) * size);
        posix_memalign((void**)&output_imag, 16, sizeof(float) * size);
        
        srand(42);
        for (int i = 0; i < size; i++) {
            input_real[i] = (float)(rand() % 65536 - 32768);
            input_imag[i] = 0.0f;
        }
        
        /* 预热 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_aligned_neon(output_real, output_imag, size, twiddle_r, twiddle_i);
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int iter = 0; iter < iterations; iter++) {
            memcpy(output_real, input_real, sizeof(float) * size);
            memcpy(output_imag, input_imag, sizeof(float) * size);
            fft_aligned_neon(output_real, output_imag, size, twiddle_r, twiddle_i);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_ms = ((end.tv_sec - start.tv_sec) * 1000.0 + 
                         (end.tv_nsec - start.tv_nsec) / 1e6) / iterations;
        
        /* 验证 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_aligned_neon(output_real, output_imag, size, twiddle_r, twiddle_i);
        ifft_aligned_neon(output_real, output_imag, size, twiddle_r, twiddle_i);
        
        double max_error = 0.0;
        for (int i = 0; i < size; i++) {
            double diff = fabs(output_real[i] - input_real[i]);
            if (diff > max_error) max_error = diff;
        }
        
        printf("| %8d | %10.4f | %7.6f |\n", size, time_ms, max_error);
        fflush(stdout);
        
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        for (int i = 0; i < max_level; i++) {
            free(twiddle_r[i]); free(twiddle_i[i]);
        }
        free(twiddle_r); free(twiddle_i);
    }
    
    printf("+----------+------------+---------+\n");
    printf("\nBenchmark Complete!\n\n");
    return 0;
}
