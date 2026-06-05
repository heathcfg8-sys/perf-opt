#ifndef FFT_LINUX_H
#define FFT_LINUX_H

#include <stdint.h>

typedef enum RetCode_e {
    K_RET_UNK_ERROR = -1,
    K_RET_OK = 0,
    K_RET_TIMEOUT,
    K_RET_INVALID_PARAM,
    K_RET_BUSY,
} RetCode_t;

/* 复数类型，供调用者使用 */
typedef struct {
    float real;
    float imag;
} complex_t;

/*
 * 对外 FFT 计算接口（使用最优版本 v5 实现）。
 * n     : FFT 点数，必须是 2 的幂次且 > 0。
 * input : 输入复数数组，长度为 n。
 * output: 输出复数数组，长度为 n。
 * 返回 K_RET_OK 表示成功。
 */
RetCode_t fft_linux_iopointer(int n, void *input, void *output);

/*
 * 简单功能自测（不打印、不计时）。
 * 内部生成随机数据，调用 fft_linux_iopointer，并通过 IFFT 验证正确性。
 */
RetCode_t fft_linux_ioself(int n);

/*
 * 性能分析自测。
 * 内部生成随机数据，调用 fft_linux_iopointer 并计时，打印耗时和验证误差。
 * 同时会展示从原始递归到 v5 各优化版本的性能对比。
 */
RetCode_t fft_linux_ioself_profiling(int n);

#endif