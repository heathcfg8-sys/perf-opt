#ifndef FFT_A_H
#define FFT_A_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define FFT_USE_NEON 1
#else
#define FFT_USE_NEON 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Common return codes used by the Linux FFT interface.
 */
typedef enum RetCode_e {
    K_RET_UNK_ERROR = -1,
    K_RET_OK = 0,
    K_RET_TIMEOUT,
    K_RET_INVALID_PARAM,
    K_RET_BUSY
} RetCode_t;

/**
 * @brief Complex number stored as two single-precision floating values.
 */
typedef struct Complex_s {
    float real;
    float imag;
} Complex_t;

/**
 * @brief Return the version label printed by the profiling executable.
 */
const char *fft_get_version_name(void);

/**
 * @brief Execute FFT in place on caller-provided real and imaginary buffers.
 *
 * @param size FFT point count. The value must be a positive power of two.
 * @param input_real Real-part input buffer. FFT real output overwrites it.
 * @param input_imag Imaginary-part input buffer. FFT imaginary output overwrites it.
 * @return K_RET_OK on success, otherwise an error code.
 */
RetCode_t fft_linux_iopointer(int size, float *input_real, float *input_imag);

/**
 * @brief Generate input data, run profiling samples, print timing and error.
 *
 * @param size FFT point count. The value must be a positive power of two.
 * @return K_RET_OK on success, otherwise an error code.
 */
RetCode_t fft_linux_ioself_profiling(int size);

/**
 * @brief Generate input data and run FFT without printing profiling output.
 *
 * @param size FFT point count. The value must be a positive power of two.
 * @return K_RET_OK on success, otherwise an error code.
 */
RetCode_t fft_linux_ioself(int size);

/**
 * @brief Execute FFT in place. Invalid parameters are ignored by this helper.
 */
void fft(float *x_real, float *x_imag, int size);

/**
 * @brief Execute inverse FFT in place. Invalid parameters are ignored by this helper.
 */
void ifft(float *x_real, float *x_imag, int size);

/**
 * @brief Release version-specific caches. No operation for versions without cache.
 */
void fft_release_cache(void);

#ifdef __cplusplus
}
#endif

#endif
