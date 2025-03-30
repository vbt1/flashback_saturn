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

// External memory region
extern unsigned char *hwram_screen;

static int _LZ_WriteVarSize(unsigned int x, unsigned char *buf)
{
    unsigned int y;
    int num_bytes, i, b;
    
    y = x >> 3;
    for(num_bytes = 5; num_bytes >= 2; --num_bytes) {
        if(y & 0xfe000000)
            break;
        y <<= 7;
    }
    
    for(i = num_bytes - 1; i >= 0; --i) {
        b = (x >> (i * 7)) & 0x7f;
        if(i > 0)
            b |= 0x80;
        *buf++ = (unsigned char) b;
    }
    
    return num_bytes;
}

static int _LZ_ReadVarSize(unsigned int *x, unsigned char *buf)
{
    unsigned int y = 0, b;
    int num_bytes = 0;
    do {
        b = (unsigned int)(*buf++);
        y = (y << 7) | (b & 0x7f);
        num_bytes++;
    } while(b & 0x80);
    *x = y;
    return num_bytes;
}

static int _LZ_Compress_Single(unsigned char *in, unsigned char *out, unsigned int insize)
{
    unsigned char marker, symbol;
    unsigned int inpos, outpos, bytesleft, i;
    unsigned int maxoffset, offset;
    unsigned int bestlength, length;
//    unsigned int histogram[256];
    unsigned int *histogram = (unsigned int *)hwram_screen;

    unsigned char *ptr1, *ptr2;
    
    if(insize < 1)
        return 0;
    
    for(i = 0; i < 256; ++i)
        histogram[i] = 0;
    for(i = 0; i < insize; ++i)
        ++histogram[in[i]];
    
    marker = 0;
    for(i = 1; i < 256; ++i) {
        if(histogram[i] < histogram[marker])
            marker = i;
    }
    
    out[0] = marker;
    inpos = 0;
    outpos = 1;
    bytesleft = insize;
    
#ifdef USE_HASH
    unsigned int *hash_table = HASH_TABLE_ADDR;
    memset(hash_table, 0xFF, HASH_TABLE_SIZE);
#endif

    unsigned int lazy_match_threshold = 6;
    
    do {
        if(inpos > LZ_MAX_OFFSET)
            maxoffset = LZ_MAX_OFFSET;
        else
            maxoffset = inpos;
        
        ptr1 = &in[inpos];
        bestlength = LZ_MIN_MATCH - 1;
        unsigned int bestoffset = 0;
        
#ifdef USE_HASH
        {
            unsigned int hash;
    #if LZ_MIN_MATCH >= 4
            hash = ((ptr1[0] << 16) ^ (ptr1[1] << 8) ^ (ptr1[2] << 4) ^ ptr1[3]) % HASH_SIZE;
    #elif LZ_MIN_MATCH == 3
            hash = ((ptr1[0] << 12) ^ (ptr1[1] << 4) ^ ptr1[2]) % HASH_SIZE;
    #else
            hash = ((ptr1[0] << 8) ^ ptr1[1]) % HASH_SIZE;
    #endif
            for(i = 0; i < HASH_CHAIN_LENGTH; i++) {
                unsigned int candidate = hash_table[hash * HASH_CHAIN_LENGTH + i];
                if(candidate == 0xFFFFFFFF || inpos - candidate > maxoffset)
                    continue;
                
                ptr2 = &in[candidate];
    #if LZ_MIN_MATCH >= 4
                if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2] && ptr1[3] == ptr2[3])
    #elif LZ_MIN_MATCH == 3
                if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2])
    #else
                if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1])
    #endif
                {
                    length = LZ_MIN_MATCH;
                    unsigned int maxlen = (insize - inpos < LZ_MAX_MATCH) ? insize - inpos : LZ_MAX_MATCH;
                    while(length < maxlen && ptr1[length] == ptr2[length])
                        length++;
                        
                    if(length > bestlength) {
                        bestlength = length;
                        bestoffset = inpos - candidate;
                    }
                }
            }
            memmove(&hash_table[hash * HASH_CHAIN_LENGTH + 1],
                    &hash_table[hash * HASH_CHAIN_LENGTH],
                    (HASH_CHAIN_LENGTH - 1) * sizeof(unsigned int));
            hash_table[hash * HASH_CHAIN_LENGTH] = inpos;
        }
#else
        for(offset = 1; offset <= maxoffset; offset++) {
            ptr2 = &ptr1[-(int)offset];
    #if LZ_MIN_MATCH >= 4
            if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2] && ptr1[3] == ptr2[3])
    #elif LZ_MIN_MATCH == 3
            if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2])
    #else
            if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1])
    #endif
            {
                unsigned int remaining = bytesleft;
                unsigned int lookahead = maxoffset - offset;
                unsigned int maxlen = (remaining < lookahead) ? remaining : lookahead;
                if(maxlen > LZ_MAX_MATCH)
                    maxlen = LZ_MAX_MATCH;
                length = LZ_MIN_MATCH;
                while(length < maxlen && ptr1[length] == ptr2[length])
                    length++;
                if(length > bestlength) {
                    bestlength = length;
                    bestoffset = offset;
                }
            }
        }
#endif

        unsigned int nextbestlength = 0;
        unsigned int nextbestoffset = 0;
        
        if (bestlength >= LZ_MIN_MATCH && bestlength < lazy_match_threshold && 
            inpos + 1 < insize && bytesleft > 1) {
            unsigned int hash;
#if LZ_MIN_MATCH >= 4
            hash = ((ptr1[0] << 16) ^ (ptr1[1] << 8) ^ (ptr1[2] << 4) ^ ptr1[3]) % HASH_SIZE;
#elif LZ_MIN_MATCH == 3
            hash = ((ptr1[0] << 12) ^ (ptr1[1] << 4) ^ ptr1[2]) % HASH_SIZE;
#else
            hash = ((ptr1[0] << 8) ^ ptr1[1]) % HASH_SIZE;
#endif
            unsigned int *saved_bucket = &hash_table[hash * HASH_CHAIN_LENGTH];
            unsigned int saved_value = saved_bucket[0];
            
            inpos++;
            bytesleft--;
            ptr1 = &in[inpos];
            
#ifdef USE_HASH
            {
                unsigned int nhash;
#if LZ_MIN_MATCH >= 4
                nhash = ((ptr1[0] << 16) ^ (ptr1[1] << 8) ^ (ptr1[2] << 4) ^ ptr1[3]) % HASH_SIZE;
#elif LZ_MIN_MATCH == 3
                nhash = ((ptr1[0] << 12) ^ (ptr1[1] << 4) ^ ptr1[2]) % HASH_SIZE;
#else
                nhash = ((ptr1[0] << 8) ^ ptr1[1]) % HASH_SIZE;
#endif
                for(i = 0; i < HASH_CHAIN_LENGTH; i++) {
                    unsigned int candidate = hash_table[nhash * HASH_CHAIN_LENGTH + i];
                    if(candidate == 0xFFFFFFFF || inpos - candidate > maxoffset)
                        continue;
                    
                    ptr2 = &in[candidate];
#if LZ_MIN_MATCH >= 4
                    if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2] && ptr1[3] == ptr2[3])
#elif LZ_MIN_MATCH == 3
                    if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2])
#else
                    if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1])
#endif
                    {
                        length = LZ_MIN_MATCH;
                        unsigned int maxlen = (insize - inpos < LZ_MAX_MATCH) ? insize - inpos : LZ_MAX_MATCH;
                        while(length < maxlen && ptr1[length] == ptr2[length])
                            length++;
                            
                        if(length > nextbestlength) {
                            nextbestlength = length;
                            nextbestoffset = inpos - candidate;
                        }
                    }
                }
            }
#endif
            
            inpos--;
            bytesleft++;
            ptr1 = &in[inpos];
            saved_bucket[0] = saved_value;
        }
        
        if (bestlength >= LZ_MIN_MATCH && (nextbestlength <= bestlength + 1 || inpos + 1 >= insize)) {
            out[outpos++] = marker;
            outpos += _LZ_WriteVarSize(bestlength, &out[outpos]);
            outpos += _LZ_WriteVarSize(bestoffset, &out[outpos]);
            inpos += bestlength;
            bytesleft -= bestlength;
        } 
        else if (nextbestlength > bestlength + 1) {
            symbol = in[inpos++];
            out[outpos++] = symbol;
            if(symbol == marker)
                out[outpos++] = 0;
            bytesleft--;
        } 
        else {
            symbol = in[inpos++];
            out[outpos++] = symbol;
            if(symbol == marker)
                out[outpos++] = 0;
            bytesleft--;
        }
    } while(bytesleft > 0);
    
    return outpos;
}

static void _LZ_Uncompress_Single(unsigned char *in, unsigned char *out, unsigned int insize)
{
    unsigned char marker, symbol;
    unsigned int i, inpos, outpos, length, offset;
    unsigned int max_outsize = 15000; // Or pass as parameter from LZW header (2984?)
    
    if(insize < 1)
        return;
    
    marker = in[0];
    inpos = 1;
    outpos = 0;
    while(inpos < insize) {
        symbol = in[inpos++];
        if(symbol == marker) {
            if(inpos < insize && in[inpos] == 0) {
                if(outpos >= max_outsize) break; // Prevent overflow
                out[outpos++] = marker;
                inpos++;
            } else {
                inpos += _LZ_ReadVarSize(&length, &in[inpos]);
                inpos += _LZ_ReadVarSize(&offset, &in[inpos]);
                if(outpos + length > max_outsize) break; // Prevent overflow
                for(i = 0; i < length; i++) {
                    out[outpos] = out[outpos - offset];
                    outpos++;
                }
            }
        } else {
            if(outpos >= max_outsize) break; // Prevent overflow
            out[outpos++] = symbol;
        }
    }
    //emu_printf("LZ stage output size: %d\n", outpos);
}

extern unsigned char *hwram_screen;

// Wrapper functions to use the LZW compression and decompression
int LZ_Compress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
    int lz_size;
     //emu_printf("LZ stage input size: %d\n", insize);   
    lz_size = _LZ_Compress_Single(in, temp_buffer, insize);
    //emu_printf("LZ stage compressed size: %d\n", lz_size);
	return lzw_compress(out, temp_buffer, lz_size);
}

void LZ_Uncompress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
	int lzw_result = lzw_decompress(temp_buffer, in, insize);
    if (lzw_result > 0) {
        //emu_printf("LZW stage decompressed size: %d\n", lzw_result);
        _LZ_Uncompress_Single(temp_buffer, out, lzw_result);
    }
}

#ifdef __cplusplus
}
#endif
