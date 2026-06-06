//
// Created by 34701 on 25-6-18.
//

/*
 * md5.h - MD5 core API.
 * Provides helpers for byte/word conversions and the main md5() entry.
 */

#ifndef KALMANFILTER_MD5_H
#define KALMANFILTER_MD5_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
// Encode a 32-bit word into 4 bytes (little-endian).
void to_bytes(uint32_t val, uint8_t *bytes);
// Decode 4 bytes into a 32-bit word (little-endian).
uint32_t to_int32(const uint8_t *bytes);
// Compute MD5 digest for the input; digest must point to 16 bytes.
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
#endif //KALMANFILTER_MD5_H
