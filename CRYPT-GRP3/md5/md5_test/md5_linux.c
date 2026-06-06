#include "md5_linux.h"
#include "timestamp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void md5_linux_compute(const uint8_t *input, size_t len, uint8_t *digest);
__attribute__((weak)) void md5_linux_compute(const uint8_t *input, size_t len, uint8_t *digest)
{
    (void)input;
    (void)len;
    if (digest != NULL) {
        memset(digest, 0, 16);
    }
}

static void md5_linux_fill_input(uint8_t *input, int horizon)
{
    for (int i = 0; i < horizon; i++) {
        input[i] = (uint8_t)(i & 0xFF);
    }
}

static void md5_linux_print_digest(const uint8_t *digest)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x", digest[i]);
    }
}

RetCode_t md5_linux_iopointer(int horizon, void *input, void *output)
{
    if (horizon <= 0 || input == NULL || output == NULL) {
        return K_RET_INVALID_PARAM;
    }

    md5_linux_compute((const uint8_t *)input, (size_t)horizon, (uint8_t *)output);
    return K_RET_OK;
}

RetCode_t md5_linux_ioself_profiling(int horizon)
{
    if (horizon <= 0) {
        return K_RET_INVALID_PARAM;
    }

    uint8_t *input_data = (uint8_t *)malloc((size_t)horizon);
    uint8_t *output_data = (uint8_t *)malloc(16);
    if (input_data == NULL || output_data == NULL) {
        free(input_data);
        free(output_data);
        return K_RET_UNK_ERROR;
    }

    md5_linux_fill_input(input_data, horizon);

    uint64_t start = timestamp();
    RetCode_t ret = md5_linux_iopointer(horizon, input_data, output_data);
    uint64_t end = timestamp();
    int64_t elapsed = timestamp_diff(start, end);

    printf("[scale=%d Bytes] MD5 time: %lld ns\n", horizon, (long long)elapsed);
    printf("Digest: ");
    md5_linux_print_digest(output_data);
    printf("\n");

    free(input_data);
    free(output_data);

    return ret;
}

RetCode_t md5_linux_ioself(int horizon)
{
    if (horizon <= 0) {
        return K_RET_INVALID_PARAM;
    }

    uint8_t *input_data = (uint8_t *)malloc((size_t)horizon);
    uint8_t *output_data = (uint8_t *)malloc(16);
    if (input_data == NULL || output_data == NULL) {
        free(input_data);
        free(output_data);
        return K_RET_UNK_ERROR;
    }

    md5_linux_fill_input(input_data, horizon);

    RetCode_t ret = md5_linux_iopointer(horizon, input_data, output_data);

    free(input_data);
    free(output_data);

    return ret;
}
