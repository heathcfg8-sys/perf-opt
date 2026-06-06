//#include <stdint.h>
//#include <stdio.h>
//#include <string.h>
//#include "sha256.h"
//#include <time.h>  // 使用 clock() 计时
//#include "sha256_interface.h"
//int main(void)
//{
//    int test_num = 14;
//    int test_scale[] = {128, 256, 512, 1024, 2*1024, 4*1024, 8*1024, 16*1024, 32*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024};
//    for( int i=0;i<test_num;i++){
//        sha256_freertos_ioself_profiling(test_scale[i]);
//    }
//    return 0;
//}

#include <stdio.h>
#include "sha256_interface.h"

int main(void)
{
    int test_num = 14;
    int test_scale[] = { 128, 256, 512, 1024, 2 * 1024, 4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024 };

    for (int i = 0; i < test_num; i++) {
        sha256_linux_ioself_profiling(test_scale[i]);
    }
    return 0;
}