#include "fft_a.h"

/**
 * @brief Test several FFT sizes and print median profiling result for each size.
 */
int main(void)
{
    int sizes[] = {256,
                   512,
                   1024,
                   2048,
                   4096,
                   8192,
                   16384,
                   32768,
                   65536,
                   1 << 17,
                   1 << 18,
                   1 << 19,
                   1 << 20};
    int size_count = (int)(sizeof(sizes) / sizeof(sizes[0]));

    printf("============================= FFT Profiling Result =============================\n");
    printf("Version: %s\n", fft_get_version_name());
    printf("Each size uses one warm-up run and seven measured runs.\n");
    printf("Input buffers are restored before every measured run.\n");
    printf("===============================================================================\n");

    for (int i = 0; i < size_count; i++) {
        RetCode_t ret = fft_linux_ioself_profiling(sizes[i]);
        if (ret != K_RET_OK) {
            printf("Profiling failed. size=%d, ret=%d\n", sizes[i], ret);
            fft_release_cache();
            return (int)ret;
        }
    }

    fft_release_cache();
    printf("===============================================================================\n");
    printf("Profiling completed.\n");
    return 0;
}
