#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>
#include <time.h>

// 声明时间结构体和函数
struct timespec timestamp();
int64_t timestamp_diff(struct timespec start, struct timespec end);

#endif // TIMESTAMP_H