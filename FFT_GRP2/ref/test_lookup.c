/**
 * FFT 查表法优化版本基准测试
 * 
 * 优化策略：
 * 1. 预计算旋转因子表 - 将sin/cos计算结果预先存储
 * 2. 运行时直接查表 - O(1)时间复杂度访问，避免重复计算
 * 3. 使用float类型 - 减少内存占用，提高缓存命中率
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TWO_PI 6.28318530717958647692f

/* ============================================================ */
/* 位反转变量 */
/* ============================================================ */

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

/* ============================================================ */
/* FFT查表法实现 */
/* ============================================================ */

static void fft_lookup(float *x_real, float *x_imag, int size,
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

static void ifft_lookup(float *x_real, float *x_imag, int size,
                        float *cos_tbl, float *sin_tbl) {
    for (int i = 0; i < size; i++) x_imag[i] = -x_imag[i];
    fft_lookup(x_real, x_imag, size, cos_tbl, sin_tbl);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

/* ============================================================ */
/* 主程序 - 基准测试 */
/* ============================================================ */

int main() {
    int sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
    int iterations = 100;
    
    printf("\n");
    printf("================================================================================\n");
    printf("             FFT Optimization Benchmark: Lookup Table Method\n");
    printf("================================================================================\n");
    printf("+----------+------------+---------+\n");
    printf("|   Size   |   Time(ms) |  Error  |\n");
    printf("+----------+------------+---------+\n");
    
    for (int s = 0; s < 8; s++) {
        int size = sizes[s];
        
        /* 初始化查找表 */
        int table_size = size / 2;
        float *cos_table = (float*)malloc(sizeof(float) * table_size);
        float *sin_table = (float*)malloc(sizeof(float) * table_size);
        
        for (int i = 0; i < table_size; i++) {
            float angle = -TWO_PI * i / size;
            cos_table[i] = cosf(angle);
            sin_table[i] = sinf(angle);
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
        fft_lookup(output_real, output_imag, size, cos_table, sin_table);
        
        /* 计时 */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int iter = 0; iter < iterations; iter++) {
            memcpy(output_real, input_real, sizeof(float) * size);
            memcpy(output_imag, input_imag, sizeof(float) * size);
            fft_lookup(output_real, output_imag, size, cos_table, sin_table);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_ms = ((end.tv_sec - start.tv_sec) * 1000.0 + 
                         (end.tv_nsec - start.tv_nsec) / 1e6) / iterations;
        
        /* 验证 */
        memcpy(output_real, input_real, sizeof(float) * size);
        memcpy(output_imag, input_imag, sizeof(float) * size);
        fft_lookup(output_real, output_imag, size, cos_table, sin_table);
        ifft_lookup(output_real, output_imag, size, cos_table, sin_table);
        
        double max_error = 0.0;
        for (int i = 0; i < size; i++) {
            double diff = fabs(output_real[i] - input_real[i]);
            if (diff > max_error) max_error = diff;
        }
        
        printf("| %8d | %10.4f | %7.6f |\n", size, time_ms, max_error);
        fflush(stdout);
        
        /* 清理 */
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        free(cos_table); free(sin_table);
    }
    
    printf("+----------+------------+---------+\n");
    printf("\nBenchmark Complete!\n\n");
    return 0;
}
