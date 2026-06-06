#include "timestamp.h"

// 获取当前时间戳
struct timespec timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

// 计算时间差，返回纳秒 (ns)
int64_t timestamp_diff(struct timespec start, struct timespec end) {
    int64_t start_ns = (int64_t)start.tv_sec * 1000000000LL + start.tv_nsec;
    int64_t end_ns = (int64_t)end.tv_sec * 1000000000LL + end.tv_nsec;
    return end_ns - start_ns;
}