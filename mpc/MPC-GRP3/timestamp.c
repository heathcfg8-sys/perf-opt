#include "timestamp.h"

struct timespec timestamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

int64_t timestamp_diff(struct timespec start, struct timespec end)
{
    int64_t sec_diff = (int64_t)(end.tv_sec - start.tv_sec);
    int64_t nsec_diff = (int64_t)(end.tv_nsec - start.tv_nsec);

    return sec_diff * 1000000000LL + nsec_diff;
}
