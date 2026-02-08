//
// Created by fsc on 25-10-5.
//
#include "aes_interface.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "aes.h"

int aes_freertos_iopointer(int scale, void* input, void* output){
    if (input == NULL || output == NULL) {
        return -1;
    }
    uint8_t *input_data = (uint8_t *)input;
    uint8_t *output_data = (uint8_t *)output;
    int times = scale / 16;
    int remainder = scale % 16;
    uint8_t key[] = {
            0x00, 0x01, 0x02, 0x03,
            0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b,
            0x0c, 0x0d, 0x0e, 0x0f,
            0x10, 0x11, 0x12, 0x13,
            0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b,
            0x1c, 0x1d, 0x1e, 0x1f};

    uint8_t *w; // expanded key

    w = aes_init(sizeof(key));

    aes_key_expansion(key, w);


    for(int i = 0; i < times; i++) {
        aes_cipher(input_data + i * 16, output_data + i * 16, w);
        aes_inv_cipher(output_data + i * 16, input_data + i * 16, w);
    }

    if (remainder > 0) {
        uint8_t input_remain[16] = {0};
        uint8_t output_remain[16] = {0};
        memcpy(input_remain, input_data + times * 16, remainder);
        aes_cipher(input_remain, output_remain, w);
        aes_inv_cipher(output_remain, input_remain, w);
        memcpy(output_data + times * 16, output_remain, remainder);
    }
    free(w);
    return 0;
}

int aes_freertos_ioself_profiling(int scale){
    uint8_t *input_data = (uint8_t *)malloc(scale);
    uint8_t *output_data = (uint8_t *)malloc(scale);

    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }

    for (int i = 0; i < scale; i++) {
        input_data[i] = i & 0xFF;
    }


    clock_t start = clock();

    int result = aes_freertos_iopointer(scale, input_data, output_data);

    clock_t end = clock();

    for (int i = 0; i < scale; i++) {
        if (input_data[i] != (i & 0xFF)) {
            printf("aes操作结果校验不通过\n");
//            for (int i = 0; i < scale; i++) {
//                printf("%02x ", output_data[i]);
//            }
//            printf("\n");
            result = -1;
            break;
        }
    }
    if (result == 0){
        printf("aes操作结果校验通过\n");
//        for (int i = 0; i < scale; i++) {
//            printf("%02x ", output_data[i]);
//        }
        printf("\n");

        double elapsed_ms = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
        printf("[规模%d]aes操作耗时: %.6f 毫秒\n", scale, elapsed_ms);
    }

    free(input_data);
    free(output_data);

    return result;
}

int aes_freertos_ioself(int scale){
    uint8_t *input_data = (uint8_t *)malloc(scale);
    uint8_t *output_data = (uint8_t *)malloc(scale);

    if (input_data == NULL || output_data == NULL) {
        if (input_data) free(input_data);
        if (output_data) free(output_data);
        return -1;
    }

    for (int i = 0; i < scale; i++) {
        input_data[i] = i & 0xFF;
    }

    int result = aes_freertos_iopointer(scale, input_data, output_data);
    free(input_data);
    free(output_data);

    return result;
}
