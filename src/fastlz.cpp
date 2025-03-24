/*
  FastLZ - Byte-aligned LZ77 compression library
  Copyright (C) 2005-2020 Ariya Hidayat <ariya.hidayat@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 2))
#define FASTLZ_LIKELY(c) (__builtin_expect(!!(c), 1))
#define FASTLZ_UNLIKELY(c) (__builtin_expect(!!(c), 0))
#else
#define FASTLZ_LIKELY(c) (c)
#define FASTLZ_UNLIKELY(c) (c)
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
#define FLZ_ARCH64
#endif

#if defined(__MSDOS__) && defined(__GNUC__)
#include <stdint-gcc.h>
#endif

#include <string.h>

static void fastlz_memmove(uint8_t* dest, const uint8_t* src, uint32_t count) {
    if ((count > 4) && (dest >= src + count)) {
        memmove(dest, src, count);
    } else {
        switch (count) {
            default: do { *dest++ = *src++; } while (--count); break;
            case 3: *dest++ = *src++;
            case 2: *dest++ = *src++;
            case 1: *dest++ = *src++;
            case 0: break;
        }
    }
}

static void fastlz_memcpy(uint8_t* dest, const uint8_t* src, uint32_t count) {
    memcpy(dest, src, count);
}

#if defined(FLZ_ARCH64)
static uint32_t flz_readu32(const void* ptr) { return *(const uint32_t*)ptr; }

static uint32_t flz_cmp(const uint8_t* p, const uint8_t* q, const uint8_t* r) {
    const uint8_t* start = p;
    if (flz_readu32(p) == flz_readu32(q)) {
        p += 4;
        q += 4;
    }
    while (q < r) if (*p++ != *q++) break;
    return p - start;
}
#else
static uint32_t flz_readu32(const void* ptr) {
    const uint8_t* p = (const uint8_t*)ptr;
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

static uint32_t flz_cmp(const uint8_t* p, const uint8_t* q, const uint8_t* r) {
    const uint8_t* start = p;
    while (q < r) if (*p++ != *q++) break;
    return p - start;
}
#endif

#define MAX_COPY 32
#define MAX_LEN 264
#define MAX_L2_DISTANCE 8191
#define MAX_FARDISTANCE 65534 /* Original FastLZ */
#define HASH_LOG 13 /* 8,192 slots, 32,768 bytes */
#define HASH_SIZE (1 << HASH_LOG)
#define HASH_MASK (HASH_SIZE - 1)

static uint16_t flz_hash(uint32_t v) {
    uint32_t h = (v * 2654435769LL) >> (32 - HASH_LOG);
    return h & HASH_MASK;
}

static void flz_smallcopy(uint8_t* dest, const uint8_t* src, uint32_t count) {
#if defined(FLZ_ARCH64)
    if (count >= 4) {
        const uint32_t* p = (const uint32_t*)src;
        uint32_t* q = (uint32_t*)dest;
        while (count > 4) {
            *q++ = *p++;
            count -= 4;
            dest += 4;
            src += 4;
        }
    }
#endif
    fastlz_memcpy(dest, src, count);
}

static void flz_maxcopy(void* dest, const void* src) {
    fastlz_memcpy((uint8_t*)dest, (uint8_t*)src, MAX_COPY);
}

static uint8_t* flz_literals(uint32_t runs, const uint8_t* src, uint8_t* dest) {
    while (runs >= MAX_COPY) {
        *dest++ = MAX_COPY - 1;
        flz_maxcopy(dest, src);
        src += MAX_COPY;
        dest += MAX_COPY;
        runs -= MAX_COPY;
    }
    if (runs > 0) {
        *dest++ = runs - 1;
        flz_smallcopy(dest, src, runs);
        dest += runs;
    }
    return dest;
}

#define FASTLZ_BOUND_CHECK(cond) if (FASTLZ_UNLIKELY(!(cond))) return 0;

static uint8_t* flz2_match(uint32_t len, uint32_t distance, uint8_t* op) {
    --distance;
    if (distance < MAX_L2_DISTANCE) {
        if (len < 7) {
            *op++ = (len << 5) + (distance >> 8);
            *op++ = (distance & 255);
        } else {
            *op++ = (7 << 5) + (distance >> 8);
            for (len -= 7; len >= 255; len -= 255) *op++ = 255;
            *op++ = len;
            *op++ = (distance & 255);
        }
    } else {
        if (len < 7) {
            distance -= MAX_L2_DISTANCE;
            *op++ = (len << 5) + 31;
            *op++ = 255;
            *op++ = distance >> 8;
            *op++ = distance & 255;
        } else {
            distance -= MAX_L2_DISTANCE;
            *op++ = (7 << 5) + 31;
            for (len -= 7; len >= 255; len -= 255) *op++ = 255;
            *op++ = len;
            *op++ = 255;
            *op++ = distance >> 8;
            *op++ = distance & 255;
        }
    }
    return op;
}

int fastlz2_compress(const void* input, int length, void* output) {
    const uint8_t* ip = (const uint8_t*)input;
    const uint8_t* ip_start = ip;
    const uint8_t* ip_bound = ip + length - 4;
    const uint8_t* ip_limit = ip + length - 12 - 1;
    uint8_t* op = (uint8_t*)output;
    uint32_t* htab = (uint32_t*)0x2aeff8; /* 8,192 slots * 4 bytes = 32,768 bytes */
    uint32_t seq, hash, hash2;

    for (hash = 0; hash < HASH_SIZE; ++hash) htab[hash] = 0;

    const uint8_t* anchor = ip;
    if (length <= 2) {
        if (length > 0) op = flz_literals(length, anchor, op);
        *(uint8_t*)output |= (1 << 5);
        return op - (uint8_t*)output;
    }
    ip += 2;

    while (FASTLZ_LIKELY(ip < ip_limit)) {
        const uint8_t* ref, *ref2;
        uint32_t distance, distance2, cmp, cmp2, len, len2;

        seq = flz_readu32(ip) & 0xffffff;
        hash = flz_hash(seq);
        hash2 = flz_hash(seq ^ 1); /* Second candidate */
        ref = ip_start + htab[hash];
        ref2 = ip_start + htab[hash2];
        htab[hash] = ip - ip_start;
        distance = ip - ref;
        distance2 = ip - ref2;
        cmp = FASTLZ_LIKELY(distance < MAX_FARDISTANCE) ? flz_readu32(ref) & 0xffffff : 0x1000000;
        cmp2 = FASTLZ_LIKELY(distance2 < MAX_FARDISTANCE) ? flz_readu32(ref2) & 0xffffff : 0x1000000;

        if (seq == cmp || seq == cmp2) {
            if ((distance >= MAX_L2_DISTANCE && (ip + 4 >= ip_bound || ref[3] != ip[3] || ref[4] != ip[4])) &&
                (distance2 >= MAX_L2_DISTANCE && (ip + 4 >= ip_bound || ref2[3] != ip[3] || ref2[4] != ip[4]))) {
                ip++;
                continue;
            }
            if (ip > anchor) op = flz_literals(ip - anchor, anchor, op);
            len = (seq == cmp) ? flz_cmp(ref + 3, ip + 3, ip_bound) : 0;
            len2 = (seq == cmp2) ? flz_cmp(ref2 + 3, ip + 3, ip_bound) : 0;
            if (len > len2 || (len == len2 && distance <= distance2)) {
                op = flz2_match(len, distance, op);
            } else {
                op = flz2_match(len2, distance2, op);
                htab[hash2] = ip - ip_start;
            }
            ip += (len > len2) ? len : len2;
            if (ip < ip_bound) {
                seq = flz_readu32(ip) & 0xffffff;
                hash = flz_hash(seq);
                htab[hash] = ip - ip_start;
                ip++;
                if (ip < ip_bound) {
                    seq >>= 8;
                    hash = flz_hash(seq);
                    htab[hash] = ip - ip_start;
                    ip++;
                    if (ip < ip_bound) {
                        seq >>= 8;
                        hash = flz_hash(seq);
                        htab[hash] = ip - ip_start;
                        ip++;
                    }
                }
            }
            anchor = ip;
        } else {
            ip++;
        }
    }

    if (anchor < ip_start + length) {
        uint32_t copy = (uint8_t*)input + length - anchor;
        op = flz_literals(copy, anchor, op);
    }

    *(uint8_t*)output |= (1 << 5);
    return op - (uint8_t*)output;
}

int fastlz2_decompress(const void* input, int length, void* output, int maxout) {
    const uint8_t* ip = (const uint8_t*)input;
    const uint8_t* ip_limit = ip + length;
    const uint8_t* ip_bound = ip + length - 2;
    uint8_t* op = (uint8_t*)output;
    uint8_t* op_limit = op + maxout;

    /* Debug: Check input validity */
    if (length < 1 || ip >= ip_limit || maxout < 1) return 0;

    uint32_t ctrl = *ip++;

    while (FASTLZ_LIKELY(ip <= ip_bound)) {
        if (ctrl < 32) { /* Literal run */
            uint32_t len = ctrl + 1;
            FASTLZ_BOUND_CHECK(ip + len <= ip_limit);
            FASTLZ_BOUND_CHECK(op + len <= op_limit);
            fastlz_memcpy(op, ip, len);
            op += len;
            ip += len;
        } else { /* Match */
            uint32_t len = (ctrl >> 5); /* 1-7 */
            uint32_t ofs = (ctrl & 31) << 8;

            FASTLZ_BOUND_CHECK(ip < ip_limit);
            ofs += *ip++;

            if (len == 7) { /* Extended length */
                len = 0;
                FASTLZ_BOUND_CHECK(ip < ip_limit);
                uint8_t code = *ip++;
                while (code == 255) {
                    len += 255;
                    FASTLZ_BOUND_CHECK(ip < ip_limit);
                    code = *ip++;
                }
                len += code;
                len += 7;
            }

            if ((ctrl & 31) == 31 && ofs == 7935) { /* Far distance */
                FASTLZ_BOUND_CHECK(ip + 1 <= ip_limit);
                ofs = (*ip++) << 8;
                ofs += *ip++;
                ofs += MAX_L2_DISTANCE;
            }

            const uint8_t* ref = op - ofs - 1;
            FASTLZ_BOUND_CHECK(ref >= (uint8_t*)output);
            FASTLZ_BOUND_CHECK(op + len <= op_limit);
            fastlz_memmove(op, ref, len);
            op += len;
        }

        if (ip <= ip_bound) {
            FASTLZ_BOUND_CHECK(ip < ip_limit);
            ctrl = *ip++;
        } else {
            break;
        }
    }

    return op - (uint8_t*)output;
}

#pragma GCC diagnostic pop