#include <string.h> // For memcpy, memmove

#ifdef __cplusplus
extern "C" {
#include "lzwlib.h"
#endif
#include "sat_mem_checker.h"
//extern void //emu_printf(const char *format, ...);
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
#define HASH_TABLE_ADDR ((unsigned int *)FRONT)
#define TEMP_BUFFER_ADDR ((unsigned char *)(FRONT + HASH_TABLE_SIZE))

// LZW-specific defines
#define LZW_DICT_SIZE 4096
#define LZW_MAX_CODE (LZW_DICT_SIZE - 1)
#define LZW_CLEAR_CODE 256
#define LZW_START_BITS 9

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
    unsigned int histogram[256];
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

// LZW-specific defines
#define LZW_DICT_SIZE 4096
#define LZW_CLEAR_CODE 256
#define LZW_END_CODE 257
#define LZW_FIRST_CODE 258
#define LZW_MAX_BITS 12
#define LZW_START_BITS 9

// Fixed LZW compression algorithm
static int _LZW_Compress(unsigned char *in, unsigned char *out, unsigned int insize) {
    //emu_printf("Starting LZW compression, insize=%u\n", insize);
    if (insize < 1) return 0;

    struct DictEntry {
        unsigned int prefix;
        unsigned char symbol;
    } *dict = (struct DictEntry *)hwram_screen;

    for (int i = 0; i < 256; i++) {
        dict[i].prefix = 0xFFFFFFFF;
        dict[i].symbol = (unsigned char)i;
    }

    unsigned int dict_size = LZW_FIRST_CODE;
    unsigned int inpos = 0;
    unsigned int outpos = 4; // Reserve space for size header
    unsigned int bit_buffer = 0;
    int bit_count = 0;
    int code_size = LZW_START_BITS;

    // Write input size header
    out[0] = (insize >> 24) & 0xFF;
    out[1] = (insize >> 16) & 0xFF;
    out[2] = (insize >> 8) & 0xFF;
    out[3] = insize & 0xFF;

    // Write initial clear code
    bit_buffer = LZW_CLEAR_CODE;
    bit_count = code_size;
    //emu_printf("Write initial clear code %u, code_size=%d\n", LZW_CLEAR_CODE, code_size);

    unsigned int current_code = in[0];
    inpos = 1;

    while (inpos < insize) {
        unsigned char next_symbol = in[inpos++];
        unsigned int next_code = 0xFFFFFFFF;

        for (unsigned int i = LZW_FIRST_CODE; i < dict_size; i++) {
            if (dict[i].prefix == current_code && dict[i].symbol == next_symbol) {
                next_code = i;
                break;
            }
        }

        if (next_code != 0xFFFFFFFF) {
            current_code = next_code;
        } else {
            // Write current_code
            if (bit_count + code_size > 32) {
                while (bit_count >= 8) {
                    out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
                    bit_count -= 8;
                    bit_buffer &= (1U << bit_count) - 1;
                }
            }
            bit_buffer = (bit_buffer << code_size) | current_code;
            bit_count += code_size;
            while (bit_count >= 8) {
                out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
                bit_count -= 8;
                bit_buffer &= (1U << bit_count) - 1;
            }

            if (dict_size < LZW_DICT_SIZE) {
                dict[dict_size].prefix = current_code;
                dict[dict_size].symbol = next_symbol;
                dict_size++;
                if (dict_size >= (1U << code_size) && code_size < LZW_MAX_BITS) {
                    code_size++;
                    //emu_printf("Increased code_size to %d at dict_size=%u\n", code_size, dict_size);
                }
            } else {
                // Clear dictionary
                bit_buffer = (bit_buffer << code_size) | LZW_CLEAR_CODE;
                bit_count += code_size;
                while (bit_count >= 8) {
                    out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
                    bit_count -= 8;
                    bit_buffer &= (1U << bit_count) - 1;
                }
                dict_size = LZW_FIRST_CODE;
                code_size = LZW_START_BITS;
            }
            current_code = next_symbol;
        }
    }

    // Write final code
    if (bit_count + code_size > 32) {
        while (bit_count >= 8) {
            out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
            bit_count -= 8;
            bit_buffer &= (1U << bit_count) - 1;
        }
    }
    bit_buffer = (bit_buffer << code_size) | current_code;
    bit_count += code_size;
    //emu_printf("Write final code %u\n", current_code);

    // Write end code
    if (bit_count + code_size > 32) {
        while (bit_count >= 8) {
            out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
            bit_count -= 8;
            bit_buffer &= (1U << bit_count) - 1;
        }
    }
    bit_buffer = (bit_buffer << code_size) | LZW_END_CODE;
    bit_count += code_size;
    //emu_printf("Write end code %u\n", LZW_END_CODE);

    // Flush remaining bits
    while (bit_count > 0) {
        out[outpos++] = (bit_buffer >> (bit_count - 8)) & 0xFF;
        bit_count -= 8;
        if (bit_count < 0) bit_count = 0; // Avoid underflow
    }

    //emu_printf("LZW compression done, compressed size=%u\n", outpos);
    return outpos;
}
// Fixed LZW decompression algorithm
static int _LZW_Uncompress(unsigned char *in, unsigned char *out, unsigned int insize, unsigned int *out_size) {
    //emu_printf("Starting LZW decompression, insize=%u\n", insize);
    if (insize < 4) {
        //emu_printf("Input too small (%u < 4)\n", insize);
        return 0;
    }

    unsigned int inpos = 0;
    *out_size = (in[inpos] << 24) | (in[inpos + 1] << 16) | (in[inpos + 2] << 8) | in[inpos + 3];
    inpos += 4;

    //emu_printf("Expected output size=%u\n", *out_size);
    if (*out_size == 0) return 0;

    struct DictEntry {
        unsigned int prefix;
        unsigned char symbol;
    } *dict = (struct DictEntry *)hwram_screen;

    unsigned char *string_buffer = TEMP_BUFFER_ADDR + TEMP_BUFFER_SIZE / 2;

    for (int i = 0; i < 256; i++) {
        dict[i].prefix = 0xFFFFFFFF;
        dict[i].symbol = (unsigned char)i;
    }

    unsigned int dict_size = LZW_FIRST_CODE;
    unsigned int outpos = 0;
    unsigned int bit_buffer = 0;
    int bit_count = 0;
    int code_size = LZW_START_BITS;

    // Read initial bits
    while (bit_count < code_size && inpos < insize) {
        bit_buffer = (bit_buffer << 8) | in[inpos++];
        bit_count += 8;
    }
    if (bit_count < code_size) {
        //emu_printf("Not enough bits for initial code\n");
        return -1;
    }

    unsigned int code = (bit_buffer >> (bit_count - code_size)) & ((1U << code_size) - 1);
    bit_count -= code_size;
    bit_buffer &= (1U << bit_count) - 1;

    if (code != LZW_CLEAR_CODE) {
        //emu_printf("Error: First code is not CLEAR_CODE (got %u)\n", code);
        return -1;
    }

    unsigned int old_code = 0xFFFFFFFF;
    unsigned char first_char = 0;

    while (inpos < insize || bit_count >= code_size) {
        while (bit_count < code_size && inpos < insize) {
            bit_buffer = (bit_buffer << 8) | in[inpos++];
            bit_count += 8;
        }
        if (bit_count < code_size) break; // End of stream

        code = (bit_buffer >> (bit_count - code_size)) & ((1U << code_size) - 1);
        bit_count -= code_size;
        bit_buffer &= (1U << bit_count) - 1;

        if (code == LZW_END_CODE) {
            //emu_printf("END_CODE reached\n");
            break;
        }

        if (code == LZW_CLEAR_CODE) {
            //emu_printf("CLEAR_CODE - Reset dictionary\n");
            dict_size = LZW_FIRST_CODE;
            code_size = LZW_START_BITS;
            old_code = 0xFFFFFFFF;
            continue;
        }

        if (code > dict_size) {
            //emu_printf("Error: Code %u exceeds dict_size %u\n", code, dict_size);
            return -3;
        }

        unsigned int string_length = 0;
        unsigned int current_code = code;

        if (code == dict_size) {
            current_code = old_code;
            while (current_code >= 256) {
                string_buffer[string_length++] = dict[current_code].symbol;
                current_code = dict[current_code].prefix;
            }
            string_buffer[string_length++] = (unsigned char)current_code;
            string_buffer[string_length++] = first_char;
        } else {
            while (current_code >= 256) {
                string_buffer[string_length++] = dict[current_code].symbol;
                current_code = dict[current_code].prefix;
            }
            string_buffer[string_length++] = (unsigned char)current_code;
        }

        first_char = string_buffer[string_length - 1];
        for (int i = string_length - 1; i >= 0; i--) {
            if (outpos < *out_size) out[outpos++] = string_buffer[i];
        }

        if (dict_size < LZW_DICT_SIZE) {
            dict[dict_size].prefix = old_code;
            dict[dict_size].symbol = first_char;
            dict_size++;
            if (dict_size >= (1U << code_size) && code_size < LZW_MAX_BITS) {
                code_size++;
                //emu_printf("Increased code_size to %d at dict_size=%u\n", code_size, dict_size);
            }
        }
        old_code = code;
    }

    //emu_printf("LZW decompression complete, decompressed size=%u\n", outpos);
    return outpos;
}

int LZ_Compress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
    int lz_size = _LZ_Compress_Single(in, temp_buffer, insize);
    return lzw_compress(out, temp_buffer, lz_size);
}

void LZ_Uncompress(unsigned char *in, unsigned char *out, unsigned int insize) {
    unsigned char *temp_buffer = TEMP_BUFFER_ADDR;
    int lzw_result = lzw_decompress(temp_buffer, in, insize);
    if (lzw_result > 0) _LZ_Uncompress_Single(temp_buffer, out, lzw_result);
}

#ifdef __cplusplus
}
#endif