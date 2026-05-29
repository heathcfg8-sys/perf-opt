#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "fft_a.h"

// 可修改FFT规模（必须是2的幂次）
#define FFT_SIZE 4096  // 例如 256, 512, 1024, 2048, 4096等

//生成随机输入数据（模拟真实信号，实部随机，虚部为0）
void generate_random_input(float *real, float *imag, int size) {
    srand((unsigned int)time(NULL));  // 以当前时间为随机种子
    for (int i = 0; i < size; i++) {
        // 实部：生成 -32768 ~ 32767 的随机浮点数（模拟PCM音频信号范围）
        real[i] = (float)(rand() % 65536 - 32768);
        // 虚部：初始化为0（真实信号通常无虚部）
        imag[i] = 0.0f;
    }
}


int main() {

    float *input_real = (float*)aligned_alloc(16, sizeof(float) * FFT_SIZE);
    float *input_imag = (float*)aligned_alloc(16, sizeof(float) * FFT_SIZE);
    float *output_real = (float*)aligned_alloc(16, sizeof(float) * FFT_SIZE);
    float *output_imag = (float*)aligned_alloc(16, sizeof(float) * FFT_SIZE);


    if (!input_real || !input_imag || !output_real || !output_imag) {
        printf("错误：内存分配失败！\n");
        // 释放已分配的内存（避免内存泄漏）
        if (input_real) free(input_real);
        if (input_imag) free(input_imag);
        if (output_real) free(output_real);
        if (output_imag) free(output_imag);
        return -1;
    }


    generate_random_input(input_real, input_imag, FFT_SIZE);
    memcpy(output_real, input_real, sizeof(float) * FFT_SIZE);
    memcpy(output_imag, input_imag, sizeof(float) * FFT_SIZE);

    printf("\n调用 fft_linux_iopointer 执行FFT...\n");
    int ret = fft_linux_iopointer(FFT_SIZE, output_real, output_imag);
    if (ret != 0) {
        printf("错误：FFT执行失败，返回码 = %d\n", ret);
        // 释放内存后退出
        free(input_real);
        free(input_imag);
        free(output_real);
        free(output_imag);
        return ret;
    }
    ifft(output_real, output_imag,FFT_SIZE);

     double max_diff = 0.0;
    for (int i = 0; i < FFT_SIZE; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }

    // 打印结果（规模、时间、误差）
    printf("Max Error: %.6f\n", max_diff);

    free(input_real);
    free(input_imag);
    free(output_real);
    free(output_imag);

    return 0;
}