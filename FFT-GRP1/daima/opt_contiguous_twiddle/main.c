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

    // 连续内存版本中，旋转因子存储区由 preCompute 在下一次调用前统一释放
    // 此处只释放二级指针表本身，避免逐层释放连续内存中的非起始地址
    if (array_r != NULL) {
        free(array_r);
        array_r = NULL;
    }
    if (array_i != NULL) {
        free(array_i);
        array_i = NULL;
    }

    printf("===================================================================\n");
    printf("Profiling completed.\n");
    return 0;
}
