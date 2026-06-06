////
//// Created by fsc on 25-10-5.
////
//#include "sha256_interface.h"
//#include "malloc.h"
//#include <time.h>
//#include "sha256.h"
//
//int sha256_freertos_iopointer(int scale, void* input, void* output){
//    sha256(input, scale, (uint8_t*)output);
//    return 0;
//}
//
//int sha256_freertos_ioself_profiling(int scale){
//    uint8_t *input_data = (uint8_t *)malloc(scale);
//    uint8_t *output_data = (uint8_t *)malloc(SHA256_SIZE_BYTES);
//    if (input_data == NULL || output_data == NULL) {
//        if (input_data) free(input_data);
//        if (output_data) free(output_data);
//        return -1;
//    }
//    for (int i = 0; i < scale; i++) {
//        input_data[i] = i & 0xFF;
//    }
//
//    clock_t start = clock();
//
//    int result = sha256_freertos_iopointer(scale, input_data, output_data);
//
//    clock_t end = clock();
//
//    printf("sha256计算结果:");
//    for (int i = 0; i < SHA256_SIZE_BYTES; i++) {
//        printf("%02x ", output_data[i]);
//    }
//    printf("\n");
//
//    double elapsed_ms = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
//    printf("[规模%d]sha256操作耗时: %.6f 毫秒\n", scale, elapsed_ms);
//
//    free(input_data);
//    free(output_data);
//
//    return result;
//}
//
//int sha256_freertos_ioself(int scale){
//    uint8_t *input_data = (uint8_t *)malloc(scale);
//    uint8_t *output_data = (uint8_t *)malloc(SHA256_SIZE_BYTES);
//    if (input_data == NULL || output_data == NULL) {
//        if (input_data) free(input_data);
//        if (output_data) free(output_data);
//        return -1;
//    }
//    for (int i = 0; i < scale; i++) {
//        input_data[i] = i & 0xFF;
//    }
//
//    int result = sha256_freertos_iopointer(scale, input_data, output_data);
//
//    free(input_data);
//    free(output_data);
//
//    return result;
//}

#include "sha256_interface.h"
#include "timestamp.h"
#include "sha256.h"
#include <stdlib.h>
#include <stdio.h>

int sha256_linux_iopointer(int scale, void* input, void* output) {
    sha256(input, scale, (uint8_t*)output);
    return 0; // 返回0表示无错误
}

int sha256_linux_ioself_profiling(int scale) {
    uint8_t* input_data = (uint8_t*)malloc(scale);
    uint8_t* output_data = (uint8_t*)malloc(SHA256_SIZE_BYTES);

    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }

    // 填充输入数据
    for (int i = 0; i < scale; i++) {
        input_data[i] = i & 0xFF;
    }

    // 开始纳秒级计时
    struct timespec start = timestamp();
    int result = sha256_linux_iopointer(scale, input_data, output_data);
    struct timespec end = timestamp();

    // 计算纳秒耗时并打印（注：为了方便你填表，我也帮你换算成了毫秒）
    int64_t elapsed_ns = timestamp_diff(start, end);
    double elapsed_ms = (double)elapsed_ns / 1000000.0;

    printf("[规模 %d Bytes] sha256 优化后耗时: %lld 纳秒 (%.6f 毫秒)\n", scale, (long long)elapsed_ns, elapsed_ms);

    free(input_data);
    free(output_data);
    return result;
}

int sha256_linux_ioself(int scale) {
    uint8_t* input_data = (uint8_t*)malloc(scale);
    uint8_t* output_data = (uint8_t*)malloc(SHA256_SIZE_BYTES);
    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }
    for (int i = 0; i < scale; i++) { input_data[i] = i & 0xFF; }
    int result = sha256_linux_iopointer(scale, input_data, output_data);
    free(input_data);
    free(output_data);
    return result;
}