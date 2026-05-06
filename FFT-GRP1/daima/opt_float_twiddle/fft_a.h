#ifndef FFT_A_H
#define FFT_A_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <arm_neon.h>
#include <string.h>

// 复数类型定义
typedef struct {
    float real;
    float imag;
} Complex;

// 全局变量声明（旋转因子数组）
extern float **array_r;
extern float **array_i;

// 函数声明
int fft_linux_iopointer(int size, float *input_real, float *input_imag);

int fft_linux_ioself_profiling(int size);

int fft_linux_ioself(int size);

void preCompute(int size);
void fft(float *x_real, float *x_imag, int size);
void ifft(float *x_real, float *x_imag, int size);

Complex complex_add(Complex a, Complex b);
Complex complex_sub(Complex a, Complex b);
Complex complex_mul(Complex a, Complex b);

#endif // MPC_LINUX_FFT_H
