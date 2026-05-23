#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>
#include <time.h>

struct timespec timestamp(void);
int64_t timestamp_diff(struct timespec start, struct timespec end);

#endif