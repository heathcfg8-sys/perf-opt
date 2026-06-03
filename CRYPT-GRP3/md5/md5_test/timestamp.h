#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

uint64_t timestamp(void);
int64_t timestamp_diff(uint64_t start, uint64_t end);

#endif
