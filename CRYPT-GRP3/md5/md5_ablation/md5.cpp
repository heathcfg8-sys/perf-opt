/*
 * md5.cpp - MD5 hash implementation with optional compile-time optimizations.
 * Controlled by feature flags in param.h.
 */
#include "md5.h"
#include "param.h"
// Constants are the integer part of the sines of integers (in radians) * 2^32.
//const uint32_t k[64] = {
//        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
//        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
//        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
//        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
//        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
//        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
//        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
//        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
//        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
//        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
//        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
//        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
//        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
//        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
//        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
//        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
//
//// r specifies the per-round shift amounts
//const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
//                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
//                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
//                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
//uint8_t *msg = NULL;
//// leftrotate function definition
//#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
//#define to_bytes(val, bytes)(*(uint32_t *)(bytes) = val)
////void to_bytes(uint32_t val, uint8_t *bytes)
////{
////    bytes[0] = (uint8_t) val;
////    bytes[1] = (uint8_t) (val >> 8);
////    bytes[2] = (uint8_t) (val >> 16);
////    bytes[3] = (uint8_t) (val >> 24);
////}
//
//uint32_t to_int32(const uint8_t *bytes)
//{
//    return (uint32_t) bytes[0]
//           | ((uint32_t) bytes[1] << 8)
//           | ((uint32_t) bytes[2] << 16)
//           | ((uint32_t) bytes[3] << 24);
//}
//
//void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest) {
//
//    // These vars will contain the hash
//    uint32_t h0, h1, h2, h3;
//
//    // Message (to prepare)
////    uint8_t *msg = NULL;
//
//    size_t new_len, offset;
//    uint32_t *w;
//    uint32_t a, b, c, d, i, f, g, temp;
//
//    // Initialize variables - simple count in nibbles:
//    h0 = 0x67452301;
//    h1 = 0xefcdab89;
//    h2 = 0x98badcfe;
//    h3 = 0x10325476;
//
//    //Pre-processing:
//    //append "1" bit to message
//    //append "0" bits until message length in bits ≡ 448 (mod 512)
//    //append length mod (2^64) to message
//
//    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
//        ;
//    if (msg == NULL){
//        msg = (uint8_t*)malloc(65535);
//    }
//    memcpy(msg, initial_msg, initial_len);
//    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
//    for (offset = initial_len + 1; offset < new_len; offset++)
//        msg[offset] = 0; // append "0" bits
//
//    // append the len in bits at the end of the buffer.
//    to_bytes(initial_len*8, msg + new_len);
//    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
//    to_bytes(initial_len>>29, msg + new_len + 4);
//
//    // Process the message in successive 512-bit chunks:
//    //for each 512-bit chunk of message:
//    #pragma GCC unroll 16
//    for(offset=0; offset<new_len; offset += (512/8)) {
//
//        // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
//        w = (uint32_t *)(msg + offset);
////        for (i = 0; i < 16; i++)
////            w[i] = to_int32(msg + offset + i*4);
//
//        // Initialize hash value for this chunk:
//        a = h0;
//        b = h1;
//        c = h2;
//        d = h3;
//
//        // Main loop:
//        for(i = 0; i<64; i++) {
//
//            if (i < 16) {
//                f = (b & c) | ((~b) & d);
//                g = i;
//            } else if (i < 32) {
//                f = (d & b) | ((~d) & c);
//                g = (5*i + 1) % 16;
//            } else if (i < 48) {
//                f = b ^ c ^ d;
//                g = (3*i + 5) % 16;
//            } else {
//                f = c ^ (b | (~d));
//                g = (7*i) % 16;
//            }
//
//            temp = d;
//            d = c;
//            c = b;
//            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
//            a = temp;
//
//        }
//
//        // Add this chunk's hash to result so far:
//        h0 += a;
//        h1 += b;
//        h2 += c;
//        h3 += d;
//
//    }
//
//    // cleanup
////    free(msg);
//
//    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
//    to_bytes(h0, digest);
//    to_bytes(h1, digest + 4);
//    to_bytes(h2, digest + 8);
//    to_bytes(h3, digest + 12);
//}

// Constants are the integer part of the sines of integers (in radians) * 2^32.
const uint32_t k[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

// r specifies the per-round shift amounts
const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

// Core boolean functions for MD5 rounds.
#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))

// One MD5 step with explicit rotate count.
#define MD5_STEP(f, a, b, c, d, x, t, s) \
    do { \
        (a) += f((b), (c), (d)) + (x) + (t); \
        (a) = LEFTROTATE((a), (s)); \
        (a) += (b); \
    } while (0)

#if defined(__aarch64__)
#define MD5_AARCH64_OPT 1
#else
#define MD5_AARCH64_OPT 0
#endif

#ifndef MD5_PAD_OPT
#define MD5_PAD_OPT 0
#endif

#ifndef MD5_ALIGN_OPT
#define MD5_ALIGN_OPT 0
#endif

#ifndef MD5_PREFETCH_OPT
#define MD5_PREFETCH_OPT 0
#endif

#ifndef MD5_UNROLL_OPT
#define MD5_UNROLL_OPT 0
#endif

#ifdef MD5MallocOpt
// Reuse a static message buffer to avoid repeated allocations.
uint8_t *msg = NULL;
size_t msg_size = 0;
#endif

#ifdef MD5MemCopyOpt
// Write length as a 32-bit word for speed.
#define to_bytes(val, bytes)(*(uint32_t *)(bytes) = val)
#else
void to_bytes(uint32_t val, uint8_t *bytes)
{
    bytes[0] = (uint8_t) val;
    bytes[1] = (uint8_t) (val >> 8);
    bytes[2] = (uint8_t) (val >> 16);
    bytes[3] = (uint8_t) (val >> 24);
}
#endif
uint32_t to_int32(const uint8_t *bytes)
{
    return (uint32_t) bytes[0]
           | ((uint32_t) bytes[1] << 8)
           | ((uint32_t) bytes[2] << 16)
           | ((uint32_t) bytes[3] << 24);
}

/*
 * Compute MD5 digest for the input message.
 * digest must point to a 16-byte output buffer.
 */
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest) {

    // These vars will contain the hash
    uint32_t h0, h1, h2, h3;

    // Message (to prepare)

    size_t new_len, offset;
    // Message schedule storage (either pointer into buffer or copied words).
#ifdef MD5MemCopyOpt
    uint32_t* w;
#else
    uint32_t w[16];
#endif
    uint32_t a, b, c, d, i, f, g, temp;

    // Initialize variables - simple count in nibbles:
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;

    //Pre-processing:
    //append "1" bit to message
    //append "0" bits until message length in bits ≡ 448 (mod 512)
    //append length mod (2^64) to message
#if MD5_PAD_OPT
    new_len = initial_len + 1;
    size_t rem = new_len & 63U;
    size_t pad = (rem <= 56U) ? (56U - rem) : (120U - rem);
    new_len += pad;
#else
    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
        ;
#endif
#ifdef MD5MallocOpt
    // Ensure the reusable buffer is large enough for padded message.
    if (msg_size < new_len + 8){
        free(msg);
        msg = NULL;
        msg_size = 0;
#if MD5_ALIGN_OPT && MD5_AARCH64_OPT
        if (posix_memalign((void **)&msg, 64, new_len + 8) != 0) {
            msg = (uint8_t*)malloc(new_len + 8);
        }
#else
        msg = (uint8_t*)malloc(new_len + 8);
#endif
        if (msg != NULL) {
            msg_size = new_len + 8;
        }
    }
#else
    // Allocate a temporary buffer for this call.
    uint8_t *msg = NULL;
#if MD5_ALIGN_OPT && MD5_AARCH64_OPT
    if (posix_memalign((void **)&msg, 64, new_len + 8) != 0) {
        msg = (uint8_t*)malloc(new_len + 8);
    }
#else
    msg = (uint8_t*)malloc(new_len + 8);
#endif
#endif
#if MD5_ALIGN_OPT
    if (msg == NULL) {
        memset(digest, 0, 16);
        return;
    }
#endif
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
#ifdef MD5LoopUnroll
    memset(msg + initial_len + 1, 0, new_len - initial_len - 1);
#else
    for (offset = initial_len + 1; offset < new_len; offset++)
        msg[offset] = 0; // append "0" bits
#endif
    // append the len in bits at the end of the buffer.
    to_bytes(initial_len*8, msg + new_len);
    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
    to_bytes(initial_len>>29, msg + new_len + 4);

    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
#ifdef MD5LoopUnroll
    #pragma GCC unroll 4
#endif
    for(offset=0; offset<new_len; offset += (512/8)) {
#if MD5_PREFETCH_OPT && MD5_AARCH64_OPT
        if (offset + (512/8) < new_len) {
            __builtin_prefetch(msg + offset + (512/8), 0, 3);
        }
#endif
#ifdef MD5MemCopyOpt
    #if MD5_ALIGN_OPT && MD5_AARCH64_OPT
        w = (uint32_t *)__builtin_assume_aligned(msg + offset, 16);
    #else
        w = (uint32_t *)(msg + offset);
    #endif
#else
        for (i = 0; i < 16; i++)
            w[i] = to_int32(msg + offset + i*4);
#endif
        // Initialize hash value for this chunk:
        a = h0;
        b = h1;
        c = h2;
        d = h3;

 // Main loop:
#if MD5_UNROLL_OPT && MD5_AARCH64_OPT
        // Fully unrolled rounds for AArch64 to avoid branches and modulo ops.
        MD5_STEP(MD5_F, a, b, c, d, w[0], 0xd76aa478, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[1], 0xe8c7b756, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[2], 0x242070db, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[3], 0xc1bdceee, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[4], 0xf57c0faf, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[5], 0x4787c62a, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[6], 0xa8304613, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[7], 0xfd469501, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[8], 0x698098d8, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[9], 0x8b44f7af, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[10], 0xffff5bb1, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[11], 0x895cd7be, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[12], 0x6b901122, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[13], 0xfd987193, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[14], 0xa679438e, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[15], 0x49b40821, 22);

        MD5_STEP(MD5_G, a, b, c, d, w[1], 0xf61e2562, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[6], 0xc040b340, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[11], 0x265e5a51, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[0], 0xe9b6c7aa, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[5], 0xd62f105d, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[10], 0x02441453, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[15], 0xd8a1e681, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[4], 0xe7d3fbc8, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[9], 0x21e1cde6, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[14], 0xc33707d6, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[3], 0xf4d50d87, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[8], 0x455a14ed, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[13], 0xa9e3e905, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[2], 0xfcefa3f8, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[7], 0x676f02d9, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[12], 0x8d2a4c8a, 20);

        MD5_STEP(MD5_H, a, b, c, d, w[5], 0xfffa3942, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[8], 0x8771f681, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[11], 0x6d9d6122, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[14], 0xfde5380c, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[1], 0xa4beea44, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[4], 0x4bdecfa9, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[7], 0xf6bb4b60, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[10], 0xbebfbc70, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[13], 0x289b7ec6, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[0], 0xeaa127fa, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[3], 0xd4ef3085, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[6], 0x04881d05, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[9], 0xd9d4d039, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[12], 0xe6db99e5, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[15], 0x1fa27cf8, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[2], 0xc4ac5665, 23);

        MD5_STEP(MD5_I, a, b, c, d, w[0], 0xf4292244, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[7], 0x432aff97, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[14], 0xab9423a7, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[5], 0xfc93a039, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[12], 0x655b59c3, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[3], 0x8f0ccc92, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[10], 0xffeff47d, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[1], 0x85845dd1, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[8], 0x6fa87e4f, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[15], 0xfe2ce6e0, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[6], 0xa3014314, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[13], 0x4e0811a1, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[4], 0xf7537e82, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[11], 0xbd3af235, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[2], 0x2ad7d2bb, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[9], 0xeb86d391, 21);
#else
        for(i = 0; i<64; i++) {

            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;

        }
#endif

        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;

    }

    // cleanup
#ifndef MD5MallocOpt

    free(msg);
#endif
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    to_bytes(h0, digest);
    to_bytes(h1, digest + 4);
    to_bytes(h2, digest + 8);
    to_bytes(h3, digest + 12);
}
