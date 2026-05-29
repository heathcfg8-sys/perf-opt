#include "md5.h"
#include <stdio.h>
#include <string.h>
#include <time.h> // for clock()
#include "md5_interface.h"

int main() {
    int test_num = 14;
    int test_scale[] = {128, 256, 512, 1024, 2*1024, 4*1024, 8*1024, 16*1024, 32*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024};
    for( int i=0;i<test_num;i++){
        md5_freertos_ioself_profiling(test_scale[i]);
    }
    return 0;
}