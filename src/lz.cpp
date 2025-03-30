#include <string.h> // For memcpy, memmove

#ifdef __cplusplus
extern "C" {
#endif
//extern void emu_printf(const char *format, ...);
#include "lzwlib.h"

#define LZ_MAX_OFFSET 32768*2
#ifndef LZ_MIN_MATCH
  #define LZ_MIN_MATCH 4
#endif
#define LZ_MAX_MATCH 4096
#define USE_HASH 1
#define HASH_SIZE 8192/4
#define HASH_CHAIN_LENGTH 16
#define HASH_TABLE_SIZE (HASH_SIZE * HASH_CHAIN_LENGTH * sizeof(unsigned int))
#define TEMP_BUFFER_SIZE 65536
#define HASH_TABLE_ADDR ((unsigned int *)0x2aeff8)
#define TEMP_BUFFER_ADDR ((unsigned char *)(0x2aeff8 + HASH_TABLE_SIZE))

static int _LZ_WriteVarSize(unsigned int x, unsigned char *buf)
{
    if (x < 128) {
        *buf = (unsigned char)x;
        return 1;
    }
    *buf++ = (unsigned char)((x & 0x7F) | 0x80);
    *buf = (unsigned char)(x >> 7);
    return 2;
}

static int _LZ_ReadVarSize(unsigned int *x, unsigned char *buf)
{
    unsigned int y = *buf++;
    if (!(y & 0x80)) {
        *x = y;
        return 1;
    }
    y = ((y & 0x7F) | (*buf << 7));
    *x = y;
    return 2;
}

static int _LZ_Compress_Single(unsigned char *in, unsigned char *out, unsigned int insize)
{
    unsigned char marker = 0;
    unsigned int inpos = 0, outpos = 1, bytesleft = insize;
    unsigned int *hash_table = HASH_TABLE_ADDR;
    
    if (insize < 1) return 0;
    
    out[0] = marker;
    memset(hash_table, 0xFF, HASH_TABLE_SIZE);
    
    while (bytesleft) {
        unsigned int maxoffset = (inpos > LZ_MAX_OFFSET) ? LZ_MAX_OFFSET : inpos;
        unsigned char *ptr1 = &in[inpos];
        unsigned int bestlength = LZ_MIN_MATCH - 1;
        unsigned int bestoffset = 0;
        
        unsigned int hash = ((ptr1[0] << 8) ^ ptr1[1]) % HASH_SIZE;
        for (int i = 0; i < HASH_CHAIN_LENGTH; i++) {
            unsigned int candidate = hash_table[hash * HASH_CHAIN_LENGTH + i];
            if (candidate == 0xFFFFFFFF || inpos - candidate > maxoffset)
                continue;
                
            unsigned char *ptr2 = &in[candidate];
            if (ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && 
                ptr1[2] == ptr2[2] && ptr1[3] == ptr2[3]) {
                unsigned int length = LZ_MIN_MATCH;
                unsigned int maxlen = (bytesleft < LZ_MAX_MATCH) ? bytesleft : LZ_MAX_MATCH;
                while (length < maxlen && ptr1[length] == ptr2[length])
                    length++;
                    
                if (length > bestlength) {
                    bestlength = length;
                    bestoffset = inpos - candidate;
                }
            }
        }
        
        if (bestlength >= LZ_MIN_MATCH) {
            out[outpos++] = marker;
            outpos += _LZ_WriteVarSize(bestlength, &out[outpos]);
            outpos += _LZ_WriteVarSize(bestoffset, &out[outpos]);
            inpos += bestlength;
            bytesleft -= bestlength;
        } else {
            unsigned char symbol = in[inpos++];
            out[outpos++] = symbol;
            if (symbol == marker)
                out[outpos++] = 0;
            bytesleft--;
        }
        
        memmove(&hash_table[hash * HASH_CHAIN_LENGTH + 1],
                &hash_table[hash * HASH_CHAIN_LENGTH],
                (HASH_CHAIN_LENGTH - 1) * sizeof(unsigned int));
        hash_table[hash * HASH_CHAIN_LENGTH] = inpos - (bestlength >= LZ_MIN_MATCH ? bestlength : 1);
    }
    
    return outpos;
}

static void _LZ_Uncompress_Single(unsigned char *in, unsigned char *out, unsigned int insize)
{
    unsigned char marker = in[0];
    unsigned int inpos = 1, outpos = 0;
    
    while (inpos < insize) {
        unsigned char symbol = in[inpos++];
        if (symbol == marker) {
            if (inpos < insize && in[inpos] == 0) {
                out[outpos++] = marker;
                inpos++;
            } else {
                unsigned int length, offset;
                inpos += _LZ_ReadVarSize(&length, &in[inpos]);
                inpos += _LZ_ReadVarSize(&offset, &in[inpos]);
                for (unsigned int i = 0; i < length; i++)
                    out[outpos + i] = out[outpos - offset + i];
                outpos += length;
            }
        } else {
            out[outpos++] = symbol;
        }
    }
}

int LZ_Compress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
    int lz_size = _LZ_Compress_Single(in, temp_buffer, insize);
    return lzw_compress(out, temp_buffer, lz_size);
}

void LZ_Uncompress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
    int lzw_result = lzw_decompress(temp_buffer, in, insize);
    if (lzw_result > 0) {
        _LZ_Uncompress_Single(temp_buffer, out, lzw_result);
    }
}

#ifdef __cplusplus
}
#endif
