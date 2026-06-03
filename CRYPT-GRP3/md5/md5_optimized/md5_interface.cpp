//
// Created by fsc on 25-10-5.
//
/*
 * md5_interface.cpp - benchmark wrappers for MD5.
 * Provides self-test and timed execution helpers.
 */
#include "md5_interface.h"
#include <stdio.h>
#include <time.h>
#include "md5.h"

int md5_freertos_iopointer(int scale, void* input, void* output){
    // Compute MD5 digest for the caller-provided buffers.
    md5((uint8_t*)input, scale, (uint8_t*)output);
    return 0;
}

int md5_freertos_ioself_profiling(int scale){
    // Allocate input/output buffers for the requested payload size.
    uint8_t *input_data = (uint8_t *)malloc(scale);
    uint8_t *output_data = (uint8_t *)malloc(16);
    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }
    // Fill input with a deterministic pattern for repeatability.
    for (int i = 0; i < scale; i++) {
        input_data[i] = i & 0xFF;
    }

    // Measure the end-to-end MD5 call.
    clock_t start = clock();

    int result = md5_freertos_iopointer(scale, input_data, output_data);

    clock_t end = clock();

    // Print digest and elapsed time.
    printf("md5计算结果:");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", output_data[i]);
    }
    printf("\n");

    double elapsed_ms = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
    printf("[规模%d]md5操作耗时: %.6f 毫秒\n", scale, elapsed_ms);

    free(input_data);
    free(output_data);

    return result;
}

int md5_freertos_ioself(int scale){
    // Same as profiling path but without timing output.
    uint8_t *input_data = (uint8_t *)malloc(scale);
    uint8_t *output_data = (uint8_t *)malloc(16);
    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }
    for (int i = 0; i < scale; i++) {
        input_data[i] = i & 0xFF;
    }

    int result = md5_freertos_iopointer(scale, input_data, output_data);

    free(input_data);
    free(output_data);

    return result;
}