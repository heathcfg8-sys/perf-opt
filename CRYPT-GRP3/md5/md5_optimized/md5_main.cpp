/*
 * md5_main.cpp - simple benchmark driver for MD5.
 * Runs md5_freertos_ioself_profiling across multiple input sizes.
 */
#include "md5.h"
#include <stdio.h>
#include <string.h>
#include <time.h> // for clock()
#include "md5_interface.h"

int main() {
    int test_num = 14;
    // Test payload sizes from 128 bytes up to 1 MiB.
    int test_scale[] = {128, 256, 512, 1024, 2*1024, 4*1024, 8*1024, 16*1024, 32*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024};
    for( int i=0;i<test_num;i++){
        md5_freertos_ioself_profiling(test_scale[i]);
    }
    return 0;
}