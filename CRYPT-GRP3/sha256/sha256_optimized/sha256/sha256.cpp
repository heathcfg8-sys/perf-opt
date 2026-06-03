#include "sha256.h"
#include <string.h> 
#ifndef _cbmc_
#define __CPROVER_assume(...) do {} while(0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FN_ static inline __attribute__((const))
#include "param.h"

    static const uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    // ================== 根据开关决定是否使用宏替换 ==================
#ifndef SHA256UnrollOpt
// 【未开启优化四】
#define FN_ static inline __attribute__((const))
    FN_ uint8_t _shb(uint32_t x, uint32_t n) {
#ifdef SHA256RremoveUselessCal
        return (x >> (n & 31));
#else
        return ((x >> (n & 31)) & 0xff);
#endif
    }
    FN_ uint32_t _shw(uint32_t x, uint32_t n) {
#ifdef SHA256RremoveUselessCal
        return (x << (n & 31));
#else
        return ((x << (n & 31)) & 0xffffffff);
#endif
    }
    FN_ uint32_t _r(uint32_t x, uint8_t n) { return ((x >> n) | _shw(x, 32 - n)); }
    FN_ uint32_t _Ch(uint32_t x, uint32_t y, uint32_t z) { return ((x & y) ^ ((~x) & z)); }
    FN_ uint32_t _Ma(uint32_t x, uint32_t y, uint32_t z) { return ((x & y) ^ (x & z) ^ (y & z)); }
    FN_ uint32_t _S0(uint32_t x) { return (_r(x, 2) ^ _r(x, 13) ^ _r(x, 22)); }
    FN_ uint32_t _S1(uint32_t x) { return (_r(x, 6) ^ _r(x, 11) ^ _r(x, 25)); }
    FN_ uint32_t _G0(uint32_t x) { return (_r(x, 7) ^ _r(x, 18) ^ (x >> 3)); }
    FN_ uint32_t _G1(uint32_t x) { return (_r(x, 17) ^ _r(x, 19) ^ (x >> 10)); }
    FN_ uint32_t _word(uint8_t* c) { return (_shw(c[0], 24) | _shw(c[1], 16) | _shw(c[2], 8) | (c[3])); }

#else
// 【开启了优化四】
#ifdef SHA256RremoveUselessCal
#define _shb(x, n) ((x) >> ((n) & 31))
#define _shw(x, n) ((x) << ((n) & 31))
#else
#define _shb(x, n) (((x) >> ((n) & 31)) & 0xff)
#define _shw(x, n) (((x) << ((n) & 31)) & 0xffffffff)
#endif
#define _r(x, n) (((x) >> (n)) | _shw(x, 32 - (n)))
#define _Ch(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define _Ma(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define _S0(x) (_r((x), 2) ^ _r((x), 13) ^ _r((x), 22))
#define _S1(x) (_r((x), 6) ^ _r((x), 11) ^ _r((x), 25))
#define _G0(x) (_r((x), 7) ^ _r((x), 18) ^ ((x) >> 3))
#define _G1(x) (_r((x), 17) ^ _r((x), 19) ^ ((x) >> 10))
#define _word(c) (_shw((c)[0], 24) | _shw((c)[1], 16) | _shw((c)[2], 8) | ((c)[3]))
#endif
// ================================================================

    static void _addbits(sha256_context* ctx, uint32_t n) {
        __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));
#ifdef SHA256BranchPred
        if (__builtin_expect(ctx->bits[0] > (0xffffffff - n), 0)) {
#else
        if (ctx->bits[0] > (0xffffffff - n)) {
#endif
#ifdef SHA256RremoveUselessCal
            ctx->bits[1] = ctx->bits[1] + 1;
#else
            ctx->bits[1] = (ctx->bits[1] + 1) & 0xFFFFFFFF;
#endif
        }
#ifdef SHA256RremoveUselessCal
        ctx->bits[0] = ctx->bits[0] + n;
#else
        ctx->bits[0] = (ctx->bits[0] + n) & 0xFFFFFFFF;
#endif
        }

    // ================== 优化四：手动宏展开的定义 ==================
#define SHA256_STEP1(i) \
    ctx->W[i] = _word(&ctx->buf[_shw(i, 2)]); \
    t[0] = h + _S1(e) + _Ch(e, f, g) + K[i] + ctx->W[i]; \
    t[1] = _S0(a) + _Ma(a, b, c); \
    h = g; g = f; f = e; e = d + t[0]; \
    d = c; c = b; b = a; a = t[0] + t[1];

#define SHA256_STEP2(i) \
    ctx->W[i] = _G1(ctx->W[i - 2])  + ctx->W[i - 7] + _G0(ctx->W[i - 15]) + ctx->W[i - 16]; \
    t[0] = h + _S1(e) + _Ch(e, f, g) + K[i] + ctx->W[i]; \
    t[1] = _S0(a) + _Ma(a, b, c); \
    h = g; g = f; f = e; e = d + t[0]; \
    d = c; c = b; b = a; a = t[0] + t[1];

    static void _hash(sha256_context * ctx) {
        __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));
        register uint32_t a, b, c, d, e, f, g, h;
        uint32_t t[2];

        a = ctx->hash[0]; b = ctx->hash[1]; c = ctx->hash[2]; d = ctx->hash[3];
        e = ctx->hash[4]; f = ctx->hash[5]; g = ctx->hash[6]; h = ctx->hash[7];

#ifdef SHA256UnrollOpt
        // 【开启优化四】：强制手动展开，步长为4，直接击穿 -O0 的限制！
        for (uint32_t i = 0; i < 16; i += 4) {
            SHA256_STEP1(i);
            SHA256_STEP1(i + 1);
            SHA256_STEP1(i + 2);
            SHA256_STEP1(i + 3);
        }
        for (uint32_t i = 16; i < 64; i += 4) {
            SHA256_STEP2(i);
            SHA256_STEP2(i + 1);
            SHA256_STEP2(i + 2);
            SHA256_STEP2(i + 3);
        }
#else
        // 【未开启优化】：老老实实的单步循环
        for (uint32_t i = 0; i < 16; i++) {
            SHA256_STEP1(i);
        }
        for (uint32_t i = 16; i < 64; i++) {
            SHA256_STEP2(i);
        }
#endif

        ctx->hash[0] += a; ctx->hash[1] += b; ctx->hash[2] += c; ctx->hash[3] += d;
        ctx->hash[4] += e; ctx->hash[5] += f; ctx->hash[6] += g; ctx->hash[7] += h;
    }

    void sha256_init(sha256_context * ctx) {
        if (ctx != NULL) {
            ctx->bits[0] = ctx->bits[1] = ctx->len = 0;
            ctx->hash[0] = 0x6a09e667; ctx->hash[1] = 0xbb67ae85;
            ctx->hash[2] = 0x3c6ef372; ctx->hash[3] = 0xa54ff53a;
            ctx->hash[4] = 0x510e527f; ctx->hash[5] = 0x9b05688c;
            ctx->hash[6] = 0x1f83d9ab; ctx->hash[7] = 0x5be0cd19;
        }
    }

    void sha256_hash(sha256_context * ctx, const void* data, size_t len) {
        const uint8_t* bytes = (const uint8_t*)data;

        if ((ctx != NULL) && (bytes != NULL) && (ctx->len < sizeof(ctx->buf))) {
            __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(bytes));
            __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));
            for (size_t i = 0; i < len; i++) {
                // ================== 优化三：数据预取 ==================
#ifdef SHA256PrefetchOpt
                if (i + 64 < len) {
                    __builtin_prefetch(&bytes[i + 64], 0, 3);
                }
#endif
                ctx->buf[ctx->len++] = bytes[i];
                if (ctx->len == sizeof(ctx->buf)) {
                    _hash(ctx);
                    _addbits(ctx, sizeof(ctx->buf) * 8);
                    ctx->len = 0;
                }
            }
        }
    }

    void sha256_done(sha256_context * ctx, uint8_t * hash) {
        register uint32_t i, j;

        if (ctx != NULL) {
            j = ctx->len % sizeof(ctx->buf);
            ctx->buf[j] = 0x80;
            // ================== 优化二：Padding填充 ==================
#ifdef SHA256PaddingOpt
            memset(&ctx->buf[j + 1], 0x00, sizeof(ctx->buf) - (j + 1));
#else
            for (i = j + 1; i < sizeof(ctx->buf); i++) { ctx->buf[i] = 0x00; }
#endif

            if (ctx->len > 55) {
                _hash(ctx);
#ifdef SHA256PaddingOpt
                memset(ctx->buf, 0x00, sizeof(ctx->buf));
#else
                for (j = 0; j < sizeof(ctx->buf); j++) { ctx->buf[j] = 0x00; }
#endif
            }

            _addbits(ctx, ctx->len * 8);
            ctx->buf[63] = _shb(ctx->bits[0], 0); ctx->buf[62] = _shb(ctx->bits[0], 8);
            ctx->buf[61] = _shb(ctx->bits[0], 16); ctx->buf[60] = _shb(ctx->bits[0], 24);
            ctx->buf[59] = _shb(ctx->bits[1], 0); ctx->buf[58] = _shb(ctx->bits[1], 8);
            ctx->buf[57] = _shb(ctx->bits[1], 16); ctx->buf[56] = _shb(ctx->bits[1], 24);
            _hash(ctx);

            if (hash != NULL) {
                for (i = 0, j = 24; i < 4; i++, j -= 8) {
                    hash[i + 0] = _shb(ctx->hash[0], j); hash[i + 4] = _shb(ctx->hash[1], j);
                    hash[i + 8] = _shb(ctx->hash[2], j); hash[i + 12] = _shb(ctx->hash[3], j);
                    hash[i + 16] = _shb(ctx->hash[4], j); hash[i + 20] = _shb(ctx->hash[5], j);
                    hash[i + 24] = _shb(ctx->hash[6], j); hash[i + 28] = _shb(ctx->hash[7], j);
                }
            }
        }
    }

    void sha256(const void* data, size_t len, uint8_t * hash) {
        sha256_context ctx;
        sha256_init(&ctx);
        sha256_hash(&ctx, data, len);
        sha256_done(&ctx, hash);
    }

#ifdef __cplusplus
    }
#endif