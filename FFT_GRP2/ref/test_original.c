/**
 * FFT Original版本基准测试 - 稳定版
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <arm_neon.h>

#define PI 3.14159265358979323846f

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

static void fft_original(float *x_real, float *x_imag, int size,
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
                    
                    float tr = wr * x_real[ip] - wi * x_imag[ip];
                    float ti = wr * x_imag[ip] + wi * x_real[ip];
                    
                    float temp_r = x_real[j + k];
                    float temp_i = x_imag[j + k];
                    
                    x_real[j + k] = temp_r + tr;
                    x_imag[j + k] = temp_i + ti;
                    x_real[ip] = temp_r - tr;
                    x_imag[ip] = temp_i - ti;
                }
            } else {
                for (int k = 0; k < le2; k += 4) {
                    float wr[4], wi[4];
                    for (int t = 0; t < 4 && k + t < le2; t++) {
                        wr[t] = (float)twiddle_r[level][k + t];
                        wi[t] = (float)twiddle_i[level][k + t];
                    }
                    
                    float32x4_t wr_v = vld1q_f32(wr);
                    float32x4_t wi_v = vld1q_f32(wi);
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

static void ifft_original(float *x_real, float *x_imag, int size,
                          double **twiddle_r, double **twiddle_i) {
    for (int i = 0; i < size; i++) x_imag[i] = -x_imag[i];
    fft_original(x_real, x_imag, size, twiddle_r, twiddle_i);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

int main() {
    int iterations = 100;
    
    printf("\n");
    printf("================================================================================\n");
    printf("                    FFT Original (Precompute + NEON)\n");
    printf("================================================================================\n");
    printf("+----------+------------+---------+\n");
    printf("|   Size   |   Time(ms) |  Error  |\n");
    printf("+----------+------------+---------+\n");
    
    /* 测试各个规模 */
    int sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
    
    for (int s = 0; s < 8; s++) {
        int size = sizes[s];
        
        /* 计算level数量 */
        int max_level = 0;
        for (int temp = 2; temp <= size; temp <<= 1) max_level++;
        
        /* 分配twiddle因子 */
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
        
        /* 分配数据 */
        float *input_real = (float*)malloc(sizeof(float) * size);
        float *input_imag = (float*)malloc(sizeof(float) * size);
        float *output_real = (float*)malloc(sizeof(float) * size);
        float *output_imag = (float*)malloc(sizeof(float) * size);
        
        srand(42);
        for (int i = 0; i < size; i++) {
            input_real[i] = (float)(rand() % 65536 - 32768);
            input_imag[i] = 0.0f;
        }
        
        /* 预热 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_original(output_real, output_imag, size, twiddle_r, twiddle_i);
        
        /* 计时 */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int iter = 0; iter < iterations; iter++) {
            memcpy(output_real, input_real, sizeof(float) * size);
            memcpy(output_imag, input_imag, sizeof(float) * size);
            fft_original(output_real, output_imag, size, twiddle_r, twiddle_i);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_ms = ((end.tv_sec - start.tv_sec) * 1000.0 + 
                         (end.tv_nsec - start.tv_nsec) / 1e6) / iterations;
        
        /* 验证 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_original(output_real, output_imag, size, twiddle_r, twiddle_i);
        ifft_original(output_real, output_imag, size, twiddle_r, twiddle_i);
        
        double max_error = 0.0;
        for (int i = 0; i < size; i++) {
            double diff = fabs(output_real[i] - input_real[i]);
            if (diff > max_error) max_error = diff;
        }
        
        printf("| %8d | %10.4f | %7.6f |\n", size, time_ms, max_error);
        fflush(stdout);
        
        /* 清理 */
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
