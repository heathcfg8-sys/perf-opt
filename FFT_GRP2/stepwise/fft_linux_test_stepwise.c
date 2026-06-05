#include "fft_linux.h"
#include <stdio.h>

int main() {
    int sizes[] = {512, 1024, 2048, 4096};
    int count = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < count; i++) {
        fft_linux_ioself_profiling(sizes[i]);
        printf("\n");
    }

    return 0;
}