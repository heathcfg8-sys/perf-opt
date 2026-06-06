#include "md5_linux.h"

#include <stdio.h>

int main(void)
{
    int scales[] = {
        128, 256, 512,
        1024, 2048, 4096,
        8192, 16384,
        32768, 65536,
        131072, 262144,
        524288, 1048576
    };
    int scale_count = (int)(sizeof(scales) / sizeof(scales[0]));

    for (int i = 0; i < scale_count; i++) {
        RetCode_t ret = md5_linux_ioself_profiling(scales[i]);
        if (ret != K_RET_OK) {
            printf("MD5 test failed at scale %d, ret=%d\n", scales[i], ret);
            return (int)ret;
        }
    }

    return 0;
}
