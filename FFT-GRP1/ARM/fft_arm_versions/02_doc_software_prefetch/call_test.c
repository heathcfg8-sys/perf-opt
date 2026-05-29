#include "fft_a.h"

#include <math.h>

#define FFT_SIZE 4096

/**
 * @brief Generate deterministic external input for fft_linux_iopointer testing.
 */
static void generate_random_input(float *real, float *imag, int size)
{
    uint32_t state = 0x12345678U ^ (uint32_t)size;

    for (int i = 0; i < size; i++) {
        state = state * 1664525U + 1013904223U;
        real[i] = (float)((int32_t)((state >> 16U) & 0xFFFFU) - 32768);
        imag[i] = 0.0F;
    }
}

/**
 * @brief Release allocated buffers before returning from main.
 */
static void release_buffers(float *input_real,
                            float *input_imag,
                            float *output_real,
                            float *output_imag)
{
    free(input_real);
    free(input_imag);
    free(output_real);
    free(output_imag);
}

/**
 * @brief Test fft_linux_iopointer with caller-owned buffers.
 */
int main(void)
{
    float *input_real = (float *)malloc(sizeof(float) * FFT_SIZE);
    float *input_imag = (float *)malloc(sizeof(float) * FFT_SIZE);
    float *output_real = (float *)malloc(sizeof(float) * FFT_SIZE);
    float *output_imag = (float *)malloc(sizeof(float) * FFT_SIZE);
    RetCode_t ret;
    double max_error = 0.0;

    if (input_real == NULL || input_imag == NULL || output_real == NULL || output_imag == NULL) {
        printf("Error: allocate memory failed.\n");
        release_buffers(input_real, input_imag, output_real, output_imag);
        return (int)K_RET_UNK_ERROR;
    }

    generate_random_input(input_real, input_imag, FFT_SIZE);
    memcpy(output_real, input_real, sizeof(float) * FFT_SIZE);
    memcpy(output_imag, input_imag, sizeof(float) * FFT_SIZE);

    printf("Version: %s\n", fft_get_version_name());
    printf("Call fft_linux_iopointer to execute FFT.\n");
    ret = fft_linux_iopointer(FFT_SIZE, output_real, output_imag);
    if (ret != K_RET_OK) {
        printf("Error: FFT failed. ret=%d\n", ret);
        release_buffers(input_real, input_imag, output_real, output_imag);
        fft_release_cache();
        return (int)ret;
    }

    ifft(output_real, output_imag, FFT_SIZE);

    for (int i = 0; i < FFT_SIZE; i++) {
        double error = fabs((double)output_real[i] - (double)input_real[i]);
        if (error > max_error) {
            max_error = error;
        }
    }

    printf("Max Error: %.6f\n", max_error);

    release_buffers(input_real, input_imag, output_real, output_imag);
    fft_release_cache();
    return 0;
}
