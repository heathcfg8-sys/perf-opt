#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return current monotonic timestamp in nanoseconds.
 */
uint64_t timestamp(void);

/**
 * @brief Return the positive nanosecond interval between two timestamps.
 */
uint64_t timestamp_diff(uint64_t start, uint64_t end);

#ifdef __cplusplus
}
#endif

#endif
