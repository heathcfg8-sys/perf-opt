#include "fft_a.h"

// ---------------- 测试主函数（按要求循环调用profiling函数） ----------------
int main() {
    // 定义输入规模范围（2的幂次，从256到2^20，可按需调整）
    int sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,1<<17,1<<18,1<<19,1<<20};
    int size_count = sizeof(sizes) / sizeof(sizes[0]);

    // 打印表头
    printf("==================== FFT Profiling Result ====================\n");
    printf("%-8s | %-12s | %-18s\n", "Size", "FFT Time (ms)", "Max Error");
    printf("===================================================================\n");

    // 循环测试不同规模
    for (int i = 0; i < size_count; i++) {
        fft_linux_ioself_profiling(sizes[i]);
    }

    // 释放旋转因子内存
    if (array_r != NULL && array_i != NULL) {
        int level = 0;
        for (int le = 2; le <= sizes[-1]; le <<= 1, level++) {
            free(array_r[level]);
            free(array_i[level]);
        }
        free(array_r);
        free(array_i);
    }

    printf("===================================================================\n");
    printf("Profiling completed.\n");
    return 0;
}
