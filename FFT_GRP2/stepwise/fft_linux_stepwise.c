#include "fft_linux.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#define PI 3.14159265358979323846

/* ================= 全局 Twiddle ================= */
static float *cos_table = NULL;
static float *sin_table = NULL;
static int table_n = 0;

static void init_twiddle(int n) {
    if (n == table_n) return;

    if (cos_table) free(cos_table);
    if (sin_table) free(sin_table);

    cos_table = (float*)aligned_alloc(32, (n/2)*sizeof(float));
    sin_table = (float*)aligned_alloc(32, (n/2)*sizeof(float));

    for (int i = 0; i < n/2; i++) {
        float angle = -2.0f * PI * i / n;
        cos_table[i] = cosf(angle);
        sin_table[i] = sinf(angle);
    }
    table_n = n;
}

static void free_twiddle(void) {
    if (cos_table) { free(cos_table); cos_table = NULL; }
    if (sin_table) { free(sin_table); sin_table = NULL; }
    table_n = 0;
}

/* ================= 位逆序 ================= */
static void bit_reverse(complex_t *x, int n) {
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (i < j) {
            complex_t tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
        int m = n >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

/* ================= 验证 ================= */
static float verify(complex_t *data, float *orig, int n);

/* ========================================================= */
/* ================= 原始递归 =============================== */
static void fft_recursive(complex_t *x, int n) {
    if (n <= 1) return;

    complex_t *even = malloc(n/2*sizeof(complex_t));
    complex_t *odd  = malloc(n/2*sizeof(complex_t));

    for (int i = 0; i < n/2; i++) {
        even[i] = x[2*i];
        odd[i]  = x[2*i+1];
    }

    fft_recursive(even, n/2);
    fft_recursive(odd, n/2);

    for (int k = 0; k < n/2; k++) {
        float angle = -2*PI*k/n;
        float c = cosf(angle);
        float s = sinf(angle);

        float tr = c*odd[k].real - s*odd[k].imag;
        float ti = s*odd[k].real + c*odd[k].imag;

        x[k].real       = even[k].real + tr;
        x[k].imag       = even[k].imag + ti;
        x[k+n/2].real   = even[k].real - tr;
        x[k+n/2].imag   = even[k].imag - ti;
    }

    free(even); free(odd);
}

/* ================= V1：迭代 =============================== */
static void fft_v1(complex_t *x, int n) {
    bit_reverse(x, n);

    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        for (int i = 0; i < n; i += len) {
            for (int k = 0; k < half; k++) {
                float angle = -2*PI*k/len;
                float wr = cosf(angle);
                float wi = sinf(angle);

                complex_t *p = &x[i+k];
                complex_t *q = &x[i+k+half];

                float tr = wr*q->real - wi*q->imag;
                float ti = wi*q->real + wr*q->imag;

                q->real = p->real - tr;
                q->imag = p->imag - ti;
                p->real += tr;
                p->imag += ti;
            }
        }
    }
}

/* ================= V2：查表 =============================== */
static void fft_v2(complex_t *x, int n) {
    bit_reverse(x, n);
    init_twiddle(n);

    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = n / len;

        for (int i = 0; i < n; i += len) {
            for (int k = 0; k < half; k++) {
                int idx = k * step;
                float wr = cos_table[idx];
                float wi = sin_table[idx];

                complex_t *p = &x[i+k];
                complex_t *q = &x[i+k+half];

                float tr = wr*q->real - wi*q->imag;
                float ti = wi*q->real + wr*q->imag;

                q->real = p->real - tr;
                q->imag = p->imag - ti;
                p->real += tr;
                p->imag += ti;
            }
        }
    }
}

/* ================= V3：对齐 ================= */
static void fft_v3(complex_t *x, int n) {
    if (((uintptr_t)x % 32) == 0) {
        fft_v2(x, n);
        return;
    }

    complex_t *buf = (complex_t*)aligned_alloc(32, n*sizeof(complex_t));
    if (!buf) return;

    memcpy(buf, x, n*sizeof(complex_t));
    fft_v2(buf, n);
    memcpy(x, buf, n*sizeof(complex_t));

    free(buf);
}

/* ================= V4：NEON =============================== */
static void fft_v4(complex_t *x, int n) {
    bit_reverse(x, n);
    init_twiddle(n);

    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = n / len;

        for (int i = 0; i < n; i += len) {
            int k = 0;
#ifdef __ARM_NEON
            for (; k <= half - 2; k += 2) {
                int idx0 = k * step;
                int idx1 = (k+1) * step;

                float32x2_t wr = {cos_table[idx0], cos_table[idx1]};
                float32x2_t wi = {sin_table[idx0], sin_table[idx1]};

                float32x2_t xr = {x[i+k].real, x[i+k+1].real};
                float32x2_t xi = {x[i+k].imag, x[i+k+1].imag};

                float32x2_t yr = {x[i+k+half].real, x[i+k+half+1].real};
                float32x2_t yi = {x[i+k+half].imag, x[i+k+half+1].imag};

                float32x2_t tr = vsub_f32(vmul_f32(wr, yr), vmul_f32(wi, yi));
                float32x2_t ti = vadd_f32(vmul_f32(wr, yi), vmul_f32(wi, yr));

                float32x2_t xr_new = vadd_f32(xr, tr);
                float32x2_t xi_new = vadd_f32(xi, ti);
                float32x2_t yr_new = vsub_f32(xr, tr);
                float32x2_t yi_new = vsub_f32(xi, ti);

                x[i+k].real = xr_new[0];
                x[i+k].imag = xi_new[0];
                x[i+k+1].real = xr_new[1];
                x[i+k+1].imag = xi_new[1];

                x[i+k+half].real = yr_new[0];
                x[i+k+half].imag = yi_new[0];
                x[i+k+half+1].real = yr_new[1];
                x[i+k+half+1].imag = yi_new[1];
            }
#endif
            for (; k < half; k++) {
                int idx = k * step;
                float wr = cos_table[idx];
                float wi = sin_table[idx];

                complex_t *p = &x[i+k];
                complex_t *q = &x[i+k+half];

                float tr = wr*q->real - wi*q->imag;
                float ti = wi*q->real + wr*q->imag;

                q->real = p->real - tr;
                q->imag = p->imag - ti;
                p->real += tr;
                p->imag += ti;
            }
        }
    }
}

/* ================= V5：Prefetch =========================== */
static void fft_v5(complex_t *x, int n) {
    bit_reverse(x, n);
    init_twiddle(n);

    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = n / len;

        for (int i = 0; i < n; i += len) {
            if (i + len < n) {
                __builtin_prefetch(&x[i + len], 0, 3);
            }

            for (int k = 0; k < half; k++) {
                int idx = k * step;
                float wr = cos_table[idx];
                float wi = sin_table[idx];

                complex_t *p = &x[i+k];
                complex_t *q = &x[i+k+half];

                float tr = wr*q->real - wi*q->imag;
                float ti = wi*q->real + wr*q->imag;

                q->real = p->real - tr;
                q->imag = p->imag - ti;
                p->real += tr;
                p->imag += ti;
            }
        }
    }
}

/* ================= 验证（IFFT 通过 v1 计算） ================= */
static float verify(complex_t *data, float *orig, int n) {
    complex_t *tmp = malloc(n*sizeof(complex_t));

    for (int i = 0; i < n; i++) {
        tmp[i].real = data[i].real;
        tmp[i].imag = -data[i].imag;
    }

    fft_v1(tmp, n);

    float max_err = 0;
    for (int i = 0; i < n; i++) {
        float r = tmp[i].real / n;
        float err = fabsf(r - orig[i]);
        if (err > max_err) max_err = err;
    }

    free(tmp);
    return max_err;
}

/* ================= 对外接口实现 ================= */

/* ---- 1. 核心计算接口（使用最优版本 v5） ---- */
RetCode_t fft_linux_iopointer(int n, void *input, void *output) {
    if (n <= 0 || (n & (n-1)) != 0)
        return K_RET_INVALID_PARAM;
    if (input == NULL || output == NULL)
        return K_RET_INVALID_PARAM;

    complex_t *in  = (complex_t*)input;
    complex_t *out = (complex_t*)output;

    if (in != out)
        memcpy(out, in, n * sizeof(complex_t));

    fft_v5(out, n);
    return K_RET_OK;
}

/* ---- 2. 简单功能自测（不计时，不打印） ---- */
RetCode_t fft_linux_ioself(int n) {
    if (n <= 0 || (n & (n-1)) != 0)
        return K_RET_INVALID_PARAM;

    complex_t *in  = (complex_t*)malloc(n * sizeof(complex_t));
    complex_t *out = (complex_t*)malloc(n * sizeof(complex_t));
    float *orig    = (float*)malloc(n * sizeof(float));

    if (!in || !out || !orig) {
        free(in); free(out); free(orig);
        return K_RET_UNK_ERROR;
    }

    for (int i = 0; i < n; i++) {
        in[i].real = (float)(rand() % 65536 - 32768);
        in[i].imag = 0.0f;
        orig[i] = in[i].real;
    }

    RetCode_t ret = fft_linux_iopointer(n, in, out);
    if (ret != K_RET_OK) {
        free(in); free(out); free(orig);
        return ret;
    }

    float max_err = verify(out, orig, n);

    free(in); free(out); free(orig);
    return (max_err < 0.1f) ? K_RET_OK : K_RET_UNK_ERROR;
}

/* ---- 3. 性能分析自测（按原始 readme 风格对比展示） ---- */
#include "timestamp.h"

/* 内部辅助：复刻原始 run_test，但不改变全局查找表 */
static void run_test(int n, void (*fft_func)(complex_t*,int), const char *name,
                     complex_t *src_data, float *orig)
{
    complex_t *in;
    complex_t *out;

    /* 只为 v3 分配对齐内存，其余用普通 malloc（保持原始行为） */
    if (strcmp(name, "v3") == 0) {
        in  = aligned_alloc(32, n*sizeof(complex_t));
        out = aligned_alloc(32, n*sizeof(complex_t));
    } else {
        in  = malloc(n*sizeof(complex_t));
        out = malloc(n*sizeof(complex_t));
    }

    /* 拷贝数据到 out（in 是临时占位，实际只用了 out） */
    for (int i = 0; i < n; i++) {
        in[i] = src_data[i];          /* 保留一份原始数据，虽然没用上 */
        out[i] = src_data[i];         /* 实际运行的数据 */
    }

    uint64_t s = timestamp();
    fft_func(out, n);
    uint64_t e = timestamp();

    printf("FFT size = %d, time = %ld ns (%s)\n", n, timestamp_diff(s,e), name);

    float err = verify(out, orig, n);
    printf("FFT size = %d, max_error = %e (%s)\n\n", n, err, name);

    free(in);
    free(out);
}

RetCode_t fft_linux_ioself_profiling(int n) {
    if (n <= 0 || (n & (n-1)) != 0)
        return K_RET_INVALID_PARAM;

    /* 清理全局查找表，使测试从干净状态开始 */
    free_twiddle();

    /* ========== 第一部分：对外接口 iopointer 性能 ========== */
    complex_t *in  = (complex_t*)malloc(n * sizeof(complex_t));
    complex_t *out = (complex_t*)malloc(n * sizeof(complex_t));
    float *orig    = (float*)malloc(n * sizeof(float));

    if (!in || !out || !orig) {
        free(in); free(out); free(orig);
        return K_RET_UNK_ERROR;
    }

    for (int i = 0; i < n; i++) {
        in[i].real = (float)(rand() % 65536 - 32768);
        in[i].imag = 0.0f;
        orig[i] = in[i].real;
    }

    /* 测试对外接口耗时 */
    memcpy(out, in, n * sizeof(complex_t));
    uint64_t start = timestamp();
    fft_linux_iopointer(n, out, out);
    uint64_t end   = timestamp();
    printf("FFT size = %d, time = %ld ns (fft_linux_iopointer, v5)\n",
           n, timestamp_diff(start, end));
    float err = verify(out, orig, n);
    printf("FFT size = %d, max_error = %e (fft_linux_iopointer)\n\n",
           n, err);

    free(in);
    free(out);
    free(orig);

    /* 再次清理查找表，为后续对比测试创造同等初始条件 */
    free_twiddle();

    /* 重新准备一份固定的测试数据（各版本共用同一组随机数） */
    complex_t *src = (complex_t*)malloc(n * sizeof(complex_t));
    orig = (float*)malloc(n * sizeof(float));
    if (!src || !orig) {
        free(src); free(orig);
        return K_RET_UNK_ERROR;
    }
    for (int i = 0; i < n; i++) {
        src[i].real = (float)(rand() % 65536 - 32768);
        src[i].imag = 0.0f;
        orig[i] = src[i].real;
    }

    /* ========== 第二部分：优化版本横向对比 ========== */
    printf("========== Original Recursive ==========\n");
    run_test(n, fft_recursive, "original", src, orig);

    printf("\n========== V1 ==========\n");
    run_test(n, fft_v1, "v1", src, orig);

    printf("\n========== V2 ==========\n");
    run_test(n, fft_v2, "v2", src, orig);

    printf("\n========== V3 ==========\n");
    run_test(n, fft_v3, "v3", src, orig);

    printf("\n========== V4 ==========\n");
    run_test(n, fft_v4, "v4", src, orig);

    printf("\n========== V5 ==========\n");
    run_test(n, fft_v5, "v5", src, orig);

    free(src);
    free(orig);

    /* 测试结束，清理全局查找表，保证下次调用不受影响 */
    free_twiddle();

    return K_RET_OK;
}