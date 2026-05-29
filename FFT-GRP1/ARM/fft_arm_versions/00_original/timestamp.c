#define _POSIX_C_SOURCE 200809L

#include "timestamp.h"

#include <time.h>

/**
 * @brief Read a monotonic clock and convert it to nanoseconds.
 */
uint64_t timestamp(void)
{
    struct timespec current_time;

#if defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
#else
    clock_gettime(CLOCK_MONOTONIC, &current_time);
#endif

    return (uint64_t)current_time.tv_sec * 1000000000ULL +
           (uint64_t)current_time.tv_nsec;
}

/**
 * @brief Compute elapsed nanoseconds while tolerating reversed inputs.
 */
uint64_t timestamp_diff(uint64_t start, uint64_t end)
{
    if (end >= start) {
        return end - start;
    }

    return 0ULL;
}
