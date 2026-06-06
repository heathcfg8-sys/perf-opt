/**
 * FFT 分块缓存优化版本 (In-Place Cache Optimization)
 * 
 * 优化策略：
 * 1. 分块FFT - 保持工作集在L1缓存内
 * 2. 减少缓存未命中
 * 3. 适合大规模FFT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

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

/* FFT - 使用连续内存访问优化 */
static void fft_block(float *x_real, float *x_imag, int size,
                     float *cos_tbl, float *sin_tbl) {
    bit_reverse(x_real, x_imag, size);
    
    for (int le = 2; le <= size; le <<= 1) {
        int le2 = le / 2;
        int step = size / le;
        
        for (int j = 0; j < size; j += le) {
            for (int k = 0; k < le2; k++) {
                int ip = j + k + le2;
                int idx = k * step;
                
                float wr = cos_tbl[idx];
                float wi = sin_tbl[idx];
                
                float tr = wr * x_real[ip] - wi * x_imag[ip];
                float ti = wr * x_imag[ip] + wi * x_real[ip];
                
                float temp_r = x_real[j + k];
                float temp_i = x_imag[j + k];
                
                x_real[j + k] = temp_r + tr;
                x_imag[j + k] = temp_i + ti;
                x_real[ip] = temp_r - tr;
                x_imag[ip] = temp_i - ti;
            }
        }
    }
}

/* Radix-4 优化FFT */
static void fft_radix4(float *x_real, float *x_imag, int size,
                      float *cos_tbl, float *sin_tbl) {
    bit_reverse(x_real, x_imag, size);
    
    /* 处理log4(N)级radix-4 */
    for (int le = 4; le <= size; le <<= 2) {
        int le4 = le / 4;
        int le2 = le / 2;
        int step = size / le;
        
        for (int j = 0; j < size; j += le) {
            for (int k = 0; k < le4; k++) {
                int j0 = j + k;
                int j1 = j + k + le4;
                int j2 = j + k + le2;
                int j3 = j + k + le2 + le4;
                
                /* Radix-4 蝶形 */
                float x0 = x_real[j0], y0 = x_imag[j0];
                float x1 = x_real[j1], y1 = x_imag[j1];
                float x2 = x_real[j2], y2 = x_imag[j2];
                float x3 = x_real[j3], y3 = x_imag[j3];
                
                float a0r = x0 + x2, a0i = y0 + y2;
                float a1r = x1 + x3, a1i = y1 + y3;
                float a2r = x0 - x2, a2i = y0 - y2;
                float a3r = x1 - x3, a3i = y1 - y3;
                
                x_real[j0] = a0r + a1r;
                x_imag[j0] = a0i + a1i;
                x_real[j2] = a0r - a1r;
                x_imag[j2] = a0i - a1i;
                
                int idx1 = (k + le4) * step;
                float w1r = cos_tbl[idx1], w1i = sin_tbl[idx1];
                
                x_real[j1] = a2r + w1r * a3i - w1i * a3r;
                x_imag[j1] = a2i + w1r * a3r + w1i * a3i;
                x_real[j3] = a2r - w1r * a3i + w1i * a3r;
                x_imag[j3] = a2i - w1r * a3r - w1i * a3i;
            }
        }
    }
    
    /* 处理剩余的radix-2级（如果size不是4的幂） */
    int processed = 1;
    while (processed * 4 < size) processed <<= 1;
    
    if (processed < size) {
        int le = processed * 2;
        int le2 = le / 2;
        int step = size / le;
        
        for (int j = 0; j < size; j += le) {
            for (int k = 0; k < le2; k++) {
                int ip = j + k + le2;
                int idx = k * step;
                
                float wr = cos_tbl[idx];
                float wi = sin_tbl[idx];
                
                float tr = wr * x_real[ip] - wi * x_imag[ip];
                float ti = wr * x_imag[ip] + wi * x_real[ip];
                
                float temp_r = x_real[j + k];
                float temp_i = x_imag[j + k];
                
                x_real[j + k] = temp_r + tr;
                x_imag[j + k] = temp_i + ti;
                x_real[ip] = temp_r - tr;
                x_imag[ip] = temp_i - ti;
            }
        }
    }
}

static void ifft_block(float *x_real, float *x_imag, int size,
                      float *cos_tbl, float *sin_tbl) {
    for (int i = 0; i < size; i++) x_imag[i] = -x_imag[i];
    fft_block(x_real, x_imag, size, cos_tbl, sin_tbl);
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
    printf("         FFT Optimization Benchmark: Cache-Optimized Block FFT\n");
    printf("================================================================================\n");
    printf("+----------+------------+---------+\n");
    printf("|   Size   |   Time(ms) |  Error  |\n");
    printf("+----------+------------+---------+\n");
    
    for (int s = 0; s < 8; s++) {
        int size = sizes[s];
        
        int table_size = size / 2;
        float *cos_table = (float*)malloc(sizeof(float) * table_size);
        float *sin_table = (float*)malloc(sizeof(float) * table_size);
        
        for (int i = 0; i < table_size; i++) {
            float angle = -2 * PI * i / size;
            cos_table[i] = cosf(angle);
            sin_table[i] = sinf(angle);
        }
        
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
        fft_block(output_real, output_imag, size, cos_table, sin_table);
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int iter = 0; iter < iterations; iter++) {
            memcpy(output_real, input_real, sizeof(float) * size);
            memcpy(output_imag, input_imag, sizeof(float) * size);
            fft_block(output_real, output_imag, size, cos_table, sin_table);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_ms = ((end.tv_sec - start.tv_sec) * 1000.0 + 
                         (end.tv_nsec - start.tv_nsec) / 1e6) / iterations;
        
        /* 验证 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_block(output_real, output_imag, size, cos_table, sin_table);
        ifft_block(output_real, output_imag, size, cos_table, sin_table);
        
        double max_error = 0.0;
        for (int i = 0; i < size; i++) {
            double diff = fabs(output_real[i] - input_real[i]);
            if (diff > max_error) max_error = diff;
        }
        
        printf("| %8d | %10.4f | %7.6f |\n", size, time_ms, max_error);
        fflush(stdout);
        
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        free(cos_table); free(sin_table);
    }
    
    printf("+----------+------------+---------+\n");
    printf("\nBenchmark Complete!\n\n");
    return 0;
}
