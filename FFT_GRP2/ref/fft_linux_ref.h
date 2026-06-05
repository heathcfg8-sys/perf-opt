/**
 * @file fft_opt.h
 * @brief FFT优化版本头文件
 */

#ifndef FFT_OPT_H
#define FFT_OPT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 类型定义
// ============================================================

typedef enum {
    OPT_ORIGINAL = 0,           /**< 原始版本 */
    OPT_LOOKUP_TABLE = 2,       /**< 查表法 */
    OPT_ENHANCED_NEON = 3       /**< NEON向量化 */
} FFT_Opt_Version;

typedef struct {
    char version_name[64];
    double fft_time_ms;
    double max_error;
    double mflops;
} FFT_Perf_Stats;

// ============================================================
// 全局变量声明
// ============================================================

extern double **array_r;
extern double **array_i;
extern float *sin_table;
extern float *cos_table;
extern int lookup_table_size;

// ============================================================
// 函数声明
// ============================================================

void* aligned_malloc(size_t size, size_t alignment);
void aligned_free(void* ptr);
const char* get_version_name(FFT_Opt_Version version);
void print_perf_stats(const char *version_name, int size, const FFT_Perf_Stats *stats);

void preCompute(int size);
void fft_original(float *x_real, float *x_imag, int size);
void ifft_original(float *x_real, float *x_imag, int size);

void init_lookup_table(int size);
void free_lookup_table(void);
void fft_lookup_table(float *x_real, float *x_imag, int size);
void ifft_lookup_table(float *x_real, float *x_imag, int size);

void fft_enhanced_neon(float *x_real, float *x_imag, int size);
void ifft_enhanced_neon(float *x_real, float *x_imag, int size);

int fft_benchmark_original(int size, FFT_Perf_Stats *stats);
int fft_benchmark_lookup_table(int size, FFT_Perf_Stats *stats);
int fft_benchmark_enhanced_neon(int size, FFT_Perf_Stats *stats);

int fft_benchmark_single(int size, FFT_Opt_Version version, FFT_Perf_Stats *stats);
void fft_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // FFT_OPT_H
