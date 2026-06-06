// ============================================================
// FFT 优化版本实现 - 修复版
// 只保留有效的优化版本
// ============================================================

#include "fft_linux_ref.h"

#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530717958647692f

// ============================================================
// 全局变量定义
// ============================================================

double **array_r = NULL;
double **array_i = NULL;

float *sin_table = NULL;
float *cos_table = NULL;
int lookup_table_size = 0;

// ============================================================
// 辅助函数
// ============================================================

void* aligned_malloc(size_t size, size_t alignment) {
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

void aligned_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// 位反转置换
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

// 计算level (le = 2^level)
static inline int get_level(int le) {
    int level = 0;
    int temp = 2;
    while (temp < le) {
        temp <<= 1;
        level++;
    }
    return level;
}

const char* get_version_name(FFT_Opt_Version version) {
    switch (version) {
        case OPT_ORIGINAL: return "Original (Precompute+NEON)";
        case OPT_LOOKUP_TABLE: return "Lookup Table";
        case OPT_ENHANCED_NEON: return "Enhanced NEON";
        default: return "Unknown";
    }
}

void print_perf_stats(const char *version_name, int size, const FFT_Perf_Stats *stats) {
    printf("| %-28s | %7d | %10.4f | %11.6f | %10.2f |\n",
           version_name, size, stats->fft_time_ms, stats->max_error, stats->mflops);
}

// ============================================================
// 预计算旋转因子
// ============================================================

void preCompute(int size) {
    if (array_r != NULL && array_i != NULL) {
        int level = 0;
        for (int le = 2; le <= size; le <<= 1, level++) {
            if (array_r[level] != NULL) free(array_r[level]);
            if (array_i[level] != NULL) free(array_i[level]);
        }
        free(array_r);
        free(array_i);
    }

    int max_level = (int)(log2(size) + 1);
    array_r = (double**)malloc(sizeof(double*) * max_level);
    array_i = (double**)malloc(sizeof(double*) * max_level);
    
    int level = 0;
    for (int le = 2; le <= size; le <<= 1, level++) {
        int le2 = le / 2;
        int step = size / le;
        array_r[level] = (double*)malloc(sizeof(double) * le2);
        array_i[level] = (double*)malloc(sizeof(double) * le2);
        for (int k = 0; k < le2; k++) {
            int idx = k * step;
            double angle = -2 * PI * idx / size;
            array_r[level][k] = cos(angle);
            array_i[level][k] = sin(angle);
        }
    }
}

// ============================================================
// V0: 原始版本 (Precompute + Partial NEON)
// ============================================================

void fft_original(float *x_real, float *x_imag, int size) {
    bit_reverse(x_real, x_imag, size);
    
    for (int le = 2; le <= size; le <<= 1) {
        int le2 = le / 2;
        int level = get_level(le);
        
        for (int j = 0; j < size; j += le) {
            if (le <= 4) {
                for (int k = 0; k < le2; k++) {
                    int ip = j + k + le2;
                    float wr = (float)array_r[level][k];
                    float wi = (float)array_i[level][k];
                    
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
                        wr[t] = (float)array_r[level][k + t];
                        wi[t] = (float)array_i[level][k + t];
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

void ifft_original(float *x_real, float *x_imag, int size) {
    for (int i = 0; i < size; i++) {
        x_imag[i] = -x_imag[i];
    }
    fft_original(x_real, x_imag, size);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

// ============================================================
// V2: 迭代 + 查表法
// ============================================================

void init_lookup_table(int size) {
    free_lookup_table();
    
    lookup_table_size = size / 2;
    sin_table = (float*)aligned_malloc(sizeof(float) * lookup_table_size, 16);
    cos_table = (float*)aligned_malloc(sizeof(float) * lookup_table_size, 16);
    
    for (int i = 0; i < lookup_table_size; i++) {
        float angle = -TWO_PI * i / size;
        sin_table[i] = sinf(angle);
        cos_table[i] = cosf(angle);
    }
}

void free_lookup_table(void) {
    if (sin_table) { free(sin_table); sin_table = NULL; }
    if (cos_table) { free(cos_table); cos_table = NULL; }
    lookup_table_size = 0;
}

void fft_lookup_table(float *x_real, float *x_imag, int size) {
    if (lookup_table_size != size / 2) {
        init_lookup_table(size);
    }
    
    bit_reverse(x_real, x_imag, size);
    
    for (int le = 2; le <= size; le <<= 1) {
        int le2 = le / 2;
        int step = lookup_table_size / le2;
        int block_offset = 0;
        
        for (int j = 0; j < size; j += le) {
            int table_idx = block_offset;
            
            for (int k = 0; k < le2; k++) {
                int ip = j + k + le2;
                
                float wr = cos_table[table_idx];
                float wi = sin_table[table_idx];
                
                float tr = wr * x_real[ip] - wi * x_imag[ip];
                float ti = wr * x_imag[ip] + wi * x_real[ip];
                
                float temp_r = x_real[j + k];
                float temp_i = x_imag[j + k];
                
                x_real[j + k] = temp_r + tr;
                x_imag[j + k] = temp_i + ti;
                x_real[ip] = temp_r - tr;
                x_imag[ip] = temp_i - ti;
                
                table_idx += step;
                if (table_idx >= lookup_table_size) table_idx -= lookup_table_size;
            }
            block_offset += step;
            if (block_offset >= lookup_table_size) block_offset -= lookup_table_size;
        }
    }
}

void ifft_lookup_table(float *x_real, float *x_imag, int size) {
    for (int i = 0; i < size; i++) {
        x_imag[i] = -x_imag[i];
    }
    fft_lookup_table(x_real, x_imag, size);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

// ============================================================
// V3: 增强NEON向量化
// ============================================================

void fft_enhanced_neon(float *x_real, float *x_imag, int size) {
    bit_reverse(x_real, x_imag, size);
    
    for (int le = 2; le <= size; le <<= 1) {
        int le2 = le / 2;
        int level = get_level(le);
        
        for (int j = 0; j < size; j += le) {
            if (le <= 4) {
                for (int k = 0; k < le2; k++) {
                    int ip = j + k + le2;
                    float wr = (float)array_r[level][k];
                    float wi = (float)array_i[level][k];
                    
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
                    int base = j + k;
                    int ip = base + le2;
                    
                    float wr[4], wi[4];
                    for (int t = 0; t < 4 && k + t < le2; t++) {
                        wr[t] = (float)array_r[level][k + t];
                        wi[t] = (float)array_i[level][k + t];
                    }
                    
                    float32x4_t wr_v = vld1q_f32(wr);
                    float32x4_t wi_v = vld1q_f32(wi);
                    float32x4_t xr_ip = vld1q_f32(&x_real[ip]);
                    float32x4_t xi_ip = vld1q_f32(&x_imag[ip]);
                    
                    float32x4_t tr_v = vmlsq_f32(vmulq_f32(wr_v, xr_ip), wi_v, xi_ip);
                    float32x4_t ti_v = vmlaq_f32(vmulq_f32(wr_v, xi_ip), wi_v, xr_ip);
                    
                    float32x4_t xr_u = vld1q_f32(&x_real[base]);
                    float32x4_t xi_u = vld1q_f32(&x_imag[base]);
                    
                    float32x4_t xr_sum = vaddq_f32(xr_u, tr_v);
                    float32x4_t xi_sum = vaddq_f32(xi_u, ti_v);
                    float32x4_t xr_diff = vsubq_f32(xr_u, tr_v);
                    float32x4_t xi_diff = vsubq_f32(xi_u, ti_v);
                    
                    vst1q_f32(&x_real[base], xr_sum);
                    vst1q_f32(&x_imag[base], xi_sum);
                    vst1q_f32(&x_real[ip], xr_diff);
                    vst1q_f32(&x_imag[ip], xi_diff);
                }
            }
        }
    }
}

void ifft_enhanced_neon(float *x_real, float *x_imag, int size) {
    for (int i = 0; i < size; i++) {
        x_imag[i] = -x_imag[i];
    }
    fft_enhanced_neon(x_real, x_imag, size);
    for (int i = 0; i < size; i++) {
        x_real[i] = x_real[i] / size;
        x_imag[i] = -x_imag[i] / size;
    }
}

// ============================================================
// 性能测试函数
// ============================================================

int fft_benchmark_original(int size, FFT_Perf_Stats *stats) {
    if ((size & (size - 1)) != 0) return -1;

    float *input_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *input_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    
    if (!input_real || !input_imag || !output_real || !output_imag) {
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        return -2;
    }

    srand(42);
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);

    struct timespec start, end;
    preCompute(size);
    clock_gettime(CLOCK_MONOTONIC, &start);
    fft_original(output_real, output_imag, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1e6;
    
    stats->fft_time_ms = elapsed;
    ifft_original(output_real, output_imag, size);
    
    double max_diff = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }
    stats->max_error = max_diff;
    
    int log2n = 0;
    for (int t = size; t > 1; t >>= 1) log2n++;
    stats->mflops = (double)size * log2n * 5 / (elapsed * 1e6);
    strncpy(stats->version_name, "Original (Precompute+NEON)", 63);
    
    free(input_real); free(input_imag); free(output_real); free(output_imag);
    return 0;
}

int fft_benchmark_lookup_table(int size, FFT_Perf_Stats *stats) {
    if ((size & (size - 1)) != 0) return -1;

    float *input_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *input_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    
    if (!input_real || !input_imag || !output_real || !output_imag) {
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        return -2;
    }

    srand(42);
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);

    struct timespec start, end;
    init_lookup_table(size);
    clock_gettime(CLOCK_MONOTONIC, &start);
    fft_lookup_table(output_real, output_imag, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1e6;
    
    stats->fft_time_ms = elapsed;
    ifft_lookup_table(output_real, output_imag, size);
    
    double max_diff = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }
    stats->max_error = max_diff;
    
    int log2n = 0;
    for (int t = size; t > 1; t >>= 1) log2n++;
    stats->mflops = (double)size * log2n * 5 / (elapsed * 1e6);
    strncpy(stats->version_name, "Lookup Table", 63);
    
    free(input_real); free(input_imag); free(output_real); free(output_imag);
    free_lookup_table();
    return 0;
}

int fft_benchmark_enhanced_neon(int size, FFT_Perf_Stats *stats) {
    if ((size & (size - 1)) != 0) return -1;

    float *input_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *input_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_real = (float*)aligned_malloc(sizeof(float) * size, 16);
    float *output_imag = (float*)aligned_malloc(sizeof(float) * size, 16);
    
    if (!input_real || !input_imag || !output_real || !output_imag) {
        free(input_real); free(input_imag); free(output_real); free(output_imag);
        return -2;
    }

    srand(42);
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);

    struct timespec start, end;
    preCompute(size);
    clock_gettime(CLOCK_MONOTONIC, &start);
    fft_enhanced_neon(output_real, output_imag, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1e6;
    
    stats->fft_time_ms = elapsed;
    ifft_enhanced_neon(output_real, output_imag, size);
    
    double max_diff = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }
    stats->max_error = max_diff;
    
    int log2n = 0;
    for (int t = size; t > 1; t >>= 1) log2n++;
    stats->mflops = (double)size * log2n * 5 / (elapsed * 1e6);
    strncpy(stats->version_name, "Enhanced NEON", 63);
    
    free(input_real); free(input_imag); free(output_real); free(output_imag);
    return 0;
}

// ============================================================
// 基准测试入口
// ============================================================

int fft_benchmark_single(int size, FFT_Opt_Version version, FFT_Perf_Stats *stats) {
    switch (version) {
        case OPT_ORIGINAL:
            return fft_benchmark_original(size, stats);
        case OPT_LOOKUP_TABLE:
            return fft_benchmark_lookup_table(size, stats);
        case OPT_ENHANCED_NEON:
            return fft_benchmark_enhanced_neon(size, stats);
        default:
            return -1;
    }
}

// ============================================================
// 清理函数
// ============================================================

void fft_cleanup(void) {
    if (sin_table) { free(sin_table); sin_table = NULL; }
    if (cos_table) { free(cos_table); cos_table = NULL; }
    lookup_table_size = 0;
    
    if (array_r != NULL && array_i != NULL) {
        int level = 0;
        for (int le = 2; le <= 1048576; le <<= 1, level++) {
            if (array_r[level] != NULL) { free(array_r[level]); array_r[level] = NULL; }
            if (array_i[level] != NULL) { free(array_i[level]); array_i[level] = NULL; }
            if (level > 20) break;
        }
        free(array_r); free(array_i);
        array_r = NULL; array_i = NULL;
    }
}
