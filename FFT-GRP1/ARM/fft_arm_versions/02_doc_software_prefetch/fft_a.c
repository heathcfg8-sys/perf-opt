#include "fft_a.h"
#include "timestamp.h"

#include <math.h>

#define FFT_PI 3.14159265358979323846
#define FFT_PROFILE_SAMPLES 7
#define FFT_PREFETCH_DISTANCE 16

/**
 * @brief Twiddle factor table for one FFT execution.
 */
typedef struct TwiddleTable_s {
    int level_count;
    double **real;
    double **imag;
} TwiddleTable_t;



/**
 * @brief Return the version label printed by test programs.
 */
const char *fft_get_version_name(void)
{
    return "02_doc_software_prefetch";
}

/**
 * @brief Release version-specific caches. Empty in versions without cached data.
 */
void fft_release_cache(void)
{
    /* This version has no persistent cache. */
}

/**
 * @brief Allocate working memory. Individual versions may change the policy.
 */
static void *fft_malloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief Check whether size is a positive power of two.
 */
static int is_power_of_two(int size)
{
    return size > 0 && (size & (size - 1)) == 0;
}

/**
 * @brief Return log2(size). The caller must pass a power-of-two size.
 */
static int get_level_count(int size)
{
    int level_count = 0;

    while (size > 1) {
        size >>= 1;
        level_count++;
    }

    return level_count;
}

/**
 * @brief Build twiddle factors for one FFT call.
 */
static RetCode_t build_twiddle_table(int size, TwiddleTable_t *table)
{
    int level_count = get_level_count(size);

    table->level_count = level_count;
    table->real = (double **)calloc((size_t)level_count, sizeof(double *));
    table->imag = (double **)calloc((size_t)level_count, sizeof(double *));

    if (table->real == NULL || table->imag == NULL) {
        return K_RET_UNK_ERROR;
    }

    for (int level = 0, le = 2; le <= size; le <<= 1, level++) {
        int le2 = le >> 1;
        table->real[level] = (double *)fft_malloc(sizeof(double) * (size_t)le2);
        table->imag[level] = (double *)fft_malloc(sizeof(double) * (size_t)le2);

        if (table->real[level] == NULL || table->imag[level] == NULL) {
            return K_RET_UNK_ERROR;
        }

        for (int k = 0; k < le2; k++) {
            double angle = -2.0 * FFT_PI * (double)k / (double)le;
            table->real[level][k] = cos(angle);
            table->imag[level][k] = sin(angle);
        }
    }

    return K_RET_OK;
}

/**
 * @brief Release all memory owned by a twiddle table.
 */
static void release_twiddle_table(TwiddleTable_t *table)
{
    if (table == NULL) {
        return;
    }

    if (table->real != NULL) {
        for (int level = 0; level < table->level_count; level++) {
            free(table->real[level]);
        }
        free(table->real);
    }

    if (table->imag != NULL) {
        for (int level = 0; level < table->level_count; level++) {
            free(table->imag[level]);
        }
        free(table->imag);
    }

    table->level_count = 0;
    table->real = NULL;
    table->imag = NULL;
}

/**
 * @brief Generate deterministic pseudo-random signal data for repeatable tests.
 */
static void generate_random_input(float *real, float *imag, int size, uint32_t seed)
{
    uint32_t state = seed;

    for (int i = 0; i < size; i++) {
        state = state * 1664525U + 1013904223U;
        real[i] = (float)((int32_t)((state >> 16U) & 0xFFFFU) - 32768);
        imag[i] = 0.0F;
    }
}

/**
 * @brief Swap two complex values stored in split real and imaginary arrays.
 */
static inline void swap_complex(float *x_real, float *x_imag, int left_index, int right_index)
{
    float temp_real = x_real[left_index];
    float temp_imag = x_imag[left_index];

    x_real[left_index] = x_real[right_index];
    x_imag[left_index] = x_imag[right_index];
    x_real[right_index] = temp_real;
    x_imag[right_index] = temp_imag;
}

/**
 * @brief Apply the original in-place bit-reversal permutation.
 */
static void bit_reverse_reorder(float *x_real, float *x_imag, int size)
{
    int reversed_index = 0;

    for (int i = 1; i < size; i++) {
        int mask = size >> 1;

        while (reversed_index >= mask) {
            reversed_index -= mask;
            mask >>= 1;
        }
        reversed_index += mask;

        if (i < reversed_index) {
            swap_complex(x_real, x_imag, i, reversed_index);
        }
    }
}


/**
 * @brief Execute one scalar butterfly.
 */
static inline void butterfly_scalar(float *x_real,
                                    float *x_imag,
                                    const TwiddleTable_t *table,
                                    int level,
                                    int left_index,
                                    int right_index,
                                    int twiddle_index)
{
    float wr = (float)table->real[level][twiddle_index];
    float wi = (float)table->imag[level][twiddle_index];
    float right_real = x_real[right_index];
    float right_imag = x_imag[right_index];
    float temp_real = wr * right_real - wi * right_imag;
    float temp_imag = wr * right_imag + wi * right_real;
    float left_real = x_real[left_index];
    float left_imag = x_imag[left_index];

    x_real[left_index] = left_real + temp_real;
    x_imag[left_index] = left_imag + temp_imag;
    x_real[right_index] = left_real - temp_real;
    x_imag[right_index] = left_imag - temp_imag;
}

/**
 * @brief Execute the iterative radix-2 FFT kernel.
 */
static void fft_execute(float *x_real, float *x_imag, int size, const TwiddleTable_t *table)
{
    bit_reverse_reorder(x_real, x_imag, size);

    for (int level = 0, le = 2; le <= size; le <<= 1, level++) {
        int le2 = le >> 1;

        for (int block_start = 0; block_start < size; block_start += le) {
#if FFT_USE_NEON
            if (le2 >= 4) {
                for (int k = 0; k < le2; k += 4) {
                    int left_index = block_start + k;
                    int right_index = left_index + le2;
                    if (k + FFT_PREFETCH_DISTANCE < le2) {
                        int next_left_index = block_start + k + FFT_PREFETCH_DISTANCE;
                        int next_right_index = next_left_index + le2;

                        __builtin_prefetch(x_real + next_left_index, 0, 1);
                        __builtin_prefetch(x_imag + next_left_index, 0, 1);
                        __builtin_prefetch(x_real + next_right_index, 0, 1);
                        __builtin_prefetch(x_imag + next_right_index, 0, 1);
                        __builtin_prefetch(table->real[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
                        __builtin_prefetch(table->imag[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
                    }

                    float wr[4] = {(float)table->real[level][k],
                                   (float)table->real[level][k + 1],
                                   (float)table->real[level][k + 2],
                                   (float)table->real[level][k + 3]};
                    float wi[4] = {(float)table->imag[level][k],
                                   (float)table->imag[level][k + 1],
                                   (float)table->imag[level][k + 2],
                                   (float)table->imag[level][k + 3]};
                    float32x4_t wr_v = vld1q_f32(wr);
                    float32x4_t wi_v = vld1q_f32(wi);
                    float32x4_t right_real_v = vld1q_f32(x_real + right_index);
                    float32x4_t right_imag_v = vld1q_f32(x_imag + right_index);
                    float32x4_t temp_real_v =
                        vmlsq_f32(vmulq_f32(wr_v, right_real_v), wi_v, right_imag_v);
                    float32x4_t temp_imag_v =
                        vmlaq_f32(vmulq_f32(wr_v, right_imag_v), wi_v, right_real_v);
                    float32x4_t left_real_v = vld1q_f32(x_real + left_index);
                    float32x4_t left_imag_v = vld1q_f32(x_imag + left_index);

                    vst1q_f32(x_real + left_index, vaddq_f32(left_real_v, temp_real_v));
                    vst1q_f32(x_imag + left_index, vaddq_f32(left_imag_v, temp_imag_v));
                    vst1q_f32(x_real + right_index, vsubq_f32(left_real_v, temp_real_v));
                    vst1q_f32(x_imag + right_index, vsubq_f32(left_imag_v, temp_imag_v));
                }
            } else
#endif
            {
                for (int k = 0; k < le2; k++) {
                    int left_index = block_start + k;
                    int right_index = left_index + le2;
                    if (k + FFT_PREFETCH_DISTANCE < le2) {
                        int next_left_index = block_start + k + FFT_PREFETCH_DISTANCE;
                        int next_right_index = next_left_index + le2;

                        __builtin_prefetch(x_real + next_left_index, 0, 1);
                        __builtin_prefetch(x_imag + next_left_index, 0, 1);
                        __builtin_prefetch(x_real + next_right_index, 0, 1);
                        __builtin_prefetch(x_imag + next_right_index, 0, 1);
                        __builtin_prefetch(table->real[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
                        __builtin_prefetch(table->imag[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
                    }

                    butterfly_scalar(x_real, x_imag, table, level, left_index, right_index, k);
                }
            }
        }
    }
}

/**
 * @brief Execute FFT in place. Invalid parameters are ignored by this helper.
 */
void fft(float *x_real, float *x_imag, int size)
{
    TwiddleTable_t table = {0};

    if (x_real == NULL || x_imag == NULL || !is_power_of_two(size)) {
        return;
    }

    if (build_twiddle_table(size, &table) != K_RET_OK) {
        release_twiddle_table(&table);
        return;
    }


    fft_execute(x_real, x_imag, size, &table);
    release_twiddle_table(&table);
}

/**
 * @brief Execute inverse FFT in place. Invalid parameters are ignored by this helper.
 */
void ifft(float *x_real, float *x_imag, int size)
{
    float inv_size;

    if (x_real == NULL || x_imag == NULL || !is_power_of_two(size)) {
        return;
    }

    for (int i = 0; i < size; i++) {
        x_imag[i] = -x_imag[i];
    }

    fft(x_real, x_imag, size);
    inv_size = 1.0F / (float)size;

    for (int i = 0; i < size; i++) {
        x_real[i] *= inv_size;
        x_imag[i] = -x_imag[i] * inv_size;
    }
}

/**
 * @brief Execute FFT in place on caller-provided buffers.
 */
RetCode_t fft_linux_iopointer(int size, float *input_real, float *input_imag)
{
    TwiddleTable_t table = {0};
    RetCode_t ret;

    if (input_real == NULL || input_imag == NULL || !is_power_of_two(size)) {
        return K_RET_INVALID_PARAM;
    }

    ret = build_twiddle_table(size, &table);
    if (ret != K_RET_OK) {
        release_twiddle_table(&table);
        return ret;
    }


    fft_execute(input_real, input_imag, size, &table);
    release_twiddle_table(&table);
    return K_RET_OK;
}

/**
 * @brief Compute the maximum real-part reconstruction error after IFFT.
 */
static double compute_max_error(const float *result_real, const float *reference_real, int size)
{
    double max_error = 0.0;

    for (int i = 0; i < size; i++) {
        double current_error = fabs((double)result_real[i] - (double)reference_real[i]);
        if (current_error > max_error) {
            max_error = current_error;
        }
    }

    return max_error;
}

/**
 * @brief Sort a small nanosecond sample array using insertion sort.
 */
static void sort_samples(uint64_t *samples, int count)
{
    for (int i = 1; i < count; i++) {
        uint64_t key = samples[i];
        int j = i - 1;

        while (j >= 0 && samples[j] > key) {
            samples[j + 1] = samples[j];
            j--;
        }
        samples[j + 1] = key;
    }
}

/**
 * @brief Generate data, run repeated timing samples, and print profiling result.
 */
RetCode_t fft_linux_ioself_profiling(int size)
{
    float *input_real;
    float *input_imag;
    float *work_real;
    float *work_imag;
    uint64_t samples[FFT_PROFILE_SAMPLES];
    RetCode_t ret;

    if (!is_power_of_two(size)) {
        return K_RET_INVALID_PARAM;
    }

    input_real = (float *)fft_malloc(sizeof(float) * (size_t)size);
    input_imag = (float *)fft_malloc(sizeof(float) * (size_t)size);
    work_real = (float *)fft_malloc(sizeof(float) * (size_t)size);
    work_imag = (float *)fft_malloc(sizeof(float) * (size_t)size);

    if (input_real == NULL || input_imag == NULL || work_real == NULL || work_imag == NULL) {
        free(input_real);
        free(input_imag);
        free(work_real);
        free(work_imag);
        return K_RET_UNK_ERROR;
    }

    generate_random_input(input_real, input_imag, size, 0x9E3779B9U ^ (uint32_t)size);

    memcpy(work_real, input_real, sizeof(float) * (size_t)size);
    memcpy(work_imag, input_imag, sizeof(float) * (size_t)size);
    ret = fft_linux_iopointer(size, work_real, work_imag);
    if (ret != K_RET_OK) {
        free(input_real);
        free(input_imag);
        free(work_real);
        free(work_imag);
        return ret;
    }

    for (int sample_index = 0; sample_index < FFT_PROFILE_SAMPLES; sample_index++) {
        uint64_t start_time;
        uint64_t end_time;

        memcpy(work_real, input_real, sizeof(float) * (size_t)size);
        memcpy(work_imag, input_imag, sizeof(float) * (size_t)size);

        start_time = timestamp();
        ret = fft_linux_iopointer(size, work_real, work_imag);
        end_time = timestamp();

        if (ret != K_RET_OK) {
            free(input_real);
            free(input_imag);
            free(work_real);
            free(work_imag);
            return ret;
        }

        samples[sample_index] = timestamp_diff(start_time, end_time);
    }

    ifft(work_real, work_imag, size);
    sort_samples(samples, FFT_PROFILE_SAMPLES);

    printf("Size: %8d | Median FFT Time: %10.4f ms | Best: %10.4f ms | Max Error: %.6f\n",
           size,
           (double)samples[FFT_PROFILE_SAMPLES / 2] / 1000000.0,
           (double)samples[0] / 1000000.0,
           compute_max_error(work_real, input_real, size));

    free(input_real);
    free(input_imag);
    free(work_real);
    free(work_imag);
    return K_RET_OK;
}

/**
 * @brief Generate data and execute FFT without profiling output.
 */
RetCode_t fft_linux_ioself(int size)
{
    float *input_real;
    float *input_imag;
    RetCode_t ret;

    if (!is_power_of_two(size)) {
        return K_RET_INVALID_PARAM;
    }

    input_real = (float *)fft_malloc(sizeof(float) * (size_t)size);
    input_imag = (float *)fft_malloc(sizeof(float) * (size_t)size);

    if (input_real == NULL || input_imag == NULL) {
        free(input_real);
        free(input_imag);
        return K_RET_UNK_ERROR;
    }

    generate_random_input(input_real, input_imag, size, 0x85EBCA6BU ^ (uint32_t)size);
    ret = fft_linux_iopointer(size, input_real, input_imag);

    free(input_real);
    free(input_imag);
    return ret;
}
