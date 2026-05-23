#include "mpc_linux.h"

#include <stdio.h>
#include <stdlib.h>

#include <omp.h>


int main(int argc, char* argv[])
{
    // 检查是否传入了命令行参数
    if (argc < 2) {
        printf("使用方法: %s horizon1 [horizon2 horizon3 ...]\n", argv[0]);
        printf("示例: %s 39\n", argv[0]);
        printf("示例: %s 39 78 156\n", argv[0]);
        return 1;
    }

    // 设置OpenMP线程数
    omp_set_num_threads(2);

#pragma omp parallel
    {
        printf("Thread %d / %d\n",
            omp_get_thread_num(),
            omp_get_num_threads());
    }


    for (int i = 1; i < argc; ++i) {
        int horizon = atoi(argv[i]);

        printf("==== Testing horizon=%d (cached) ====\n", horizon);

        mpc_linux_ioself_profiling_cached(horizon);

        printf("\n");
    }


    return 0;
}