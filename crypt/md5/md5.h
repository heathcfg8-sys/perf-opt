//
// Created by 34701 on 25-6-18.
//

#ifndef KALMANFILTER_MD5_H
#define KALMANFILTER_MD5_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
void to_bytes(uint32_t val, uint8_t *bytes);
uint32_t to_int32(const uint8_t *bytes);
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
#endif //KALMANFILTER_MD5_H
