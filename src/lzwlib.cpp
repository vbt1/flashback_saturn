////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                  Copyright (c) 2016-2020 David Bryant                  //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "lzwlib.h"
//extern void emu_printf(const char *format, ...);

/* This library implements the LZW general-purpose data compression algorithm.
 * The algorithm was originally described as a hardware implementation by
 * Terry Welsh here:
 *
 *   Welch, T.A. “A Technique for High-Performance Data Compression.”
 *   IEEE Computer 17,6 (June 1984), pp. 8-19.
 *
 * Since then there have been enumerable refinements and variations on the
 * basic technique, and this implementation is no different. The target of
 * the present implementation is embedded systems, and so emphasis was placed
 * on simplicity, fast execution, and minimal RAM usage.
 * 
 * This is a streaming compressor in that the data is not divided into blocks
 * and no context information like dictionaries or Huffman tables are sent
 * ahead of the compressed data (except for one byte to signal the maximum
 * bit depth). This limits the maximum possible compression ratio compared to
 * algorithms that significantly preprocess the data, but with the help of
 * some enhancements to the LZW algorithm (described below) it is able to
 * compress better than the UNIX "compress" utility (which is also LZW) and
 * is in fact closer to and sometimes beats the compression level of "gzip".
 *
 * The symbols are stored in "adjusted binary" which provides somewhat better
 * compression, with virtually no speed penalty, compared to the fixed word
 * sizes normally used. These are sometimes called "phased-in" binary codes
 * and their use in LZW is described here:
 *
 *   R. N. Horspool, "Improving LZW (data compression algorithm)", Data
 *   Compression Conference, pp. 332-341, 1991.
 *
 * Earlier versions of this compressor would reset as soon as the dictionary
 * became full to ensure good performance on heterogenous data (such as tar
 * files or executable images). While trivial to implement, this is not
 * particularly efficient with homogeneous data (or in general) because we
 * spend a lot of time sending short symbols where the compression is poor.
 *
 * This newer version utilizes a technique such that once the dictionary is
 * full, we restart at the beginning and recycle only those codes that were
 * seen only once. We know this because they are not referenced by longer
 * strings, and are easy to replace in the dictionary for the same reason.
 * Since they have only been seen once it's also more likely that we will
 * be replacing them with a more common string, and this is especially
 * true if the data characteristics are changing.
 *
 * Replacing string codes in this manner has the interesting side effect that
 * some older shorter strings that the removed strings were based on will
 * possibly become unreferenced themselves and be recycled on the next pass.
 * In this way, the entire dictionary constantly "churns" based on the
 * incoming stream, thereby improving and adapting to optimal compression.
 *
 * Even with this technique there is still a possibility that a sudden change
 * in the data characteristics will appear, resulting in significant negative
 * compression (up to 100% for 16-bit codes). To detect this case we generate
 * an exponentially decaying average of the current compression ratio and reset
 * when this hits about 1.06, which limits worst case inflation to about 8%.
 *
 * The maximum symbol size is configurable on the encode side (from 9 bits to
 * 16 bits) and determines the RAM footprint required by both sides and, to a
 * large extent, the compression performance. This information is communicated
 * to the decoder in the first stream byte so that it can allocate accordingly.
 * The RAM requirements are as follows:
 *
 *    maximum    encoder RAM   decoder RAM
 *  symbol size  requirement   requirement
 * -----------------------------------------
 *     9-bit      4096 bytes    2368 bytes
 *    10-bit      8192 bytes    4992 bytes
 *    11-bit     16384 bytes   10240 bytes
 *    12-bit     32768 bytes   20736 bytes
 *    13-bit     65536 bytes   41728 bytes
 *    14-bit    131072 bytes   83712 bytes
 *    15-bit    262144 bytes  167680 bytes
 *    16-bit    524288 bytes  335616 bytes
 *
 * This implementation uses malloc(), but obviously an embedded version could
 * use static arrays instead if desired (assuming that the maxbits was
 * controlled outside).
 */

#define NULL_CODE       65535   // indicates a NULL prefix (must be unsigned short)
#define CLEAR_CODE      256     // code to flush dictionary and restart decoder
#define FIRST_STRING    257     // code of first dictionary string

/* This macro determines the number of bits required to represent the given value,
 * not counting the implied MSB. For GNU C it will use the provided built-in,
 * otherwise a comparison tree is employed. Note that in the non-GNU case, only
 * values up to 65535 (15 bits) are supported.
 */

#ifdef __GNUC__
#define CODE_BITS(n) (31 - __builtin_clz(n))
#else
#define CODE_BITS(n) ((n) < 4096 ?                                      \
            ((n) < 1024  ? 8  + ((n) >= 512)  : 10 + ((n) >= 2048)) :   \
            ((n) < 16384 ? 12 + ((n) >= 8192) : 14 + ((n) >= 32768)))
#endif

/* This macro writes the adjusted-binary symbol "code" given the maximum
 * symbol "maxcode". A macro is used here just to avoid the duplication in
 * the lzw_compress() function. The idea is that if "maxcode" is not one
 * less than a power of two (which it rarely will be) then this code can
 * often send fewer bits that would be required with a fixed-sized code.
 *
 * For example, the first code we send will have a "maxcode" of 257, so
 * every "code" would normally consume 9 bits. But with adjusted binary we
 * can actually represent any code from 0 to 253 with just 8 bits -- only
 * the 4 codes from 254 to 257 take 9 bits.
 */
extern unsigned char *hwram_screen;

typedef struct {
    unsigned short first_reference, next_reference, back_reference;
    unsigned char terminator;
} encoder_entry_t;

#if 0
#define WRITE_CODE(code, maxcode) do {                               \
    unsigned int code_bits = CODE_BITS(maxcode);                    \
    unsigned int extras = (2 << code_bits) - (maxcode) - 1;         \
    if ((code) < extras) {                                          \
        shifter |= ((code) << bits);                                \
        bits += code_bits;                                          \
    }                                                               \
    else {                                                          \
        shifter |= ((((code) + extras) >> 1) << bits);              \
        bits += code_bits;                                          \
        shifter |= ((((code) + extras) & 1) << bits++);             \
    }                                                               \
    do {                                                            \
        dst[dst_index++] = (shifter & 0xFF);  /* Write byte to output */ \
        shifter >>= 8;                                              \
        output_bytes += 256;                                        \
    } while ((bits -= 8) >= 8);                                     \
} while (0)
#else
static inline void write_code(unsigned char *dst, size_t *dst_index, unsigned int *shifter, unsigned int *bits, unsigned int code, unsigned int maxcode, unsigned int *output_bytes) {
    unsigned int code_bits = CODE_BITS(maxcode);
    unsigned int extras = (2 << code_bits) - (maxcode) - 1;
    if (code < extras) {
        *shifter |= (code << *bits);
        *bits += code_bits;
    } else {
        *shifter |= (((code + extras) >> 1) << *bits);
        *bits += code_bits;
        *shifter |= (((code + extras) & 1) << (*bits)++);
    }
    while (*bits >= 8) {
        dst[(*dst_index)++] = (*shifter & 0xFF);
        *shifter >>= 8;
        *output_bytes += 256;
        *bits -= 8;
    }
}
#endif
int lzw_compress(unsigned char *dst, unsigned char *src, size_t src_input_size) {
    unsigned int maxcode = FIRST_STRING, next_string = FIRST_STRING, prefix = NULL_CODE, total_codes;
    unsigned int dictionary_full = 0, available_entries, max_available_entries, max_available_code;
    unsigned int input_bytes = 65536, output_bytes = 65536;
    unsigned int shifter = 0, bits = 0;
    encoder_entry_t *dictionary;
    size_t src_index = 0, dst_index = 0;
    int c;
    int maxbits = 12;
    total_codes = 1 << maxbits;
    dictionary = (encoder_entry_t *)hwram_screen;
    max_available_entries = total_codes - FIRST_STRING - 1;
    max_available_code = total_codes - 2;
    available_entries = max_available_entries;
    memset(dictionary, 0, sizeof(encoder_entry_t) << 8);
    dst[dst_index++] = (maxbits - 9);
    while (src_index < src_input_size) {
//	emu_printf("src_index %d /%d \n",src_index,src_input_size);
        c = src[src_index++];
        unsigned int cti;
        input_bytes += 256;
        if (prefix == NULL_CODE) {
            prefix = c;
            continue;
        }
        memset(dictionary + next_string, 0, sizeof(encoder_entry_t));
        if ((cti = dictionary[prefix].first_reference)) {
            while (1) {
                if (dictionary[cti].terminator == c) {
                    prefix = cti;
                    break;
                } else if (!dictionary[cti].next_reference) {
                    dictionary[cti].next_reference = next_string;
                    dictionary[next_string].back_reference = cti;
                    cti = 0;
                    break;
                } else {
                    cti = dictionary[cti].next_reference;
                }
            }
        } else {
            dictionary[prefix].first_reference = next_string;
            dictionary[next_string].back_reference = prefix;
            if (prefix >= FIRST_STRING) available_entries--;
        }
        if (!cti) {
//            WRITE_CODE(prefix, maxcode);
			write_code(dst, &dst_index, &shifter, &bits, prefix, maxcode, &output_bytes);
            dictionary[next_string].terminator = c;
            prefix = c;
            if (!dictionary_full) {
                dictionary_full = (++next_string > max_available_code);
                maxcode++;
            }
            if (dictionary_full) {
                for (next_string++; next_string <= max_available_code || (next_string = FIRST_STRING); next_string++) {
                    if (!dictionary[next_string].first_reference)
                        break;
                }
                cti = dictionary[next_string].back_reference;
                if (dictionary[cti].first_reference == next_string) {
                    dictionary[cti].first_reference = dictionary[next_string].next_reference;
                    if (!dictionary[cti].first_reference && cti >= FIRST_STRING)
                        available_entries++;
                } else if (dictionary[cti].next_reference == next_string) {
                    dictionary[cti].next_reference = dictionary[next_string].next_reference;
                }
                if (dictionary[next_string].next_reference)
                    dictionary[dictionary[next_string].next_reference].back_reference = cti;
                if (available_entries < 16 || available_entries * 100 < max_available_entries) {
//                    WRITE_CODE(CLEAR_CODE, maxcode);
					write_code(dst, &dst_index, &shifter, &bits, CLEAR_CODE, maxcode, &output_bytes);
                    memset(dictionary, 0, 256 * sizeof(encoder_entry_t));
                    available_entries = max_available_entries;
                    next_string = maxcode = FIRST_STRING;
                    input_bytes = output_bytes = 65536;
                    dictionary_full = 0;
                }
            }
            if (output_bytes > input_bytes + (input_bytes >> 4)) {
//                WRITE_CODE(CLEAR_CODE, maxcode);
				write_code(dst, &dst_index, &shifter, &bits, CLEAR_CODE, maxcode, &output_bytes);
                memset(dictionary, 0, 256 * sizeof(encoder_entry_t));
                available_entries = max_available_entries;
                next_string = maxcode = FIRST_STRING;
                input_bytes = output_bytes = 65536;
                dictionary_full = 0;
            } else {
                output_bytes -= output_bytes >> 8;
                input_bytes -= input_bytes >> 8;
            }
        }
    }
    if (prefix != NULL_CODE) {
//        WRITE_CODE(prefix, maxcode);
		write_code(dst, &dst_index, &shifter, &bits, prefix, maxcode, &output_bytes);
        if (!dictionary_full)
            maxcode++;
    }
//    WRITE_CODE(maxcode, maxcode);
	write_code(dst, &dst_index, &shifter, &bits, maxcode, maxcode, &output_bytes);
    if (bits) {
        dst[dst_index++] = shifter;
    }
    return dst_index;
}


/* LZW decompression function. Bytes (8-bit) are read and written through callbacks. The
 * "maxbits" parameter is read as the first byte in the stream and controls how much memory
 * is allocated for decoding. A return value of EOF from the "src" callback terminates the
 * decompression process (although this should not normally occur). A non-zero return value
 * indicates an error, which in this case can be a bad "maxbits" read from the stream, a
 * failed malloc(), or if an EOF is read from the input stream before the decompression
 * terminates naturally with END_CODE. There are contexts (void pointers) that are passed
 * to the callbacks to easily facilitate multiple instances of the decompression operation
 * (but simple applications can ignore these).
 */

typedef struct {
    unsigned char terminator, extra_references;
    unsigned short prefix;
} decoder_entry_t;

int lzw_decompress (unsigned char *dst, unsigned char *src, size_t src_size)
{
    unsigned int maxcode = FIRST_STRING, next_string = FIRST_STRING - 1, prefix = CLEAR_CODE;
    unsigned int dictionary_full = 0, max_available_code, total_codes;
    unsigned int shifter = 0, bits = 0, read_byte, i;
    unsigned char *reverse_buffer, *referenced;
    decoder_entry_t *dictionary;
    size_t src_index = 0;
    unsigned char *original_dst = dst;
//emu_printf("input size %d\n",src_size);
	read_byte = src[src_index++];
//    if (src_index >= src_size || (read_byte = src[src_index++]) & 0xf8) // sanitize first byte
//	{
//emu_printf("error 1\n");
//        return -1;
//	}
    total_codes = 512 << (read_byte & 0x7);
    max_available_code = total_codes - 2;
    dictionary = (decoder_entry_t *)hwram_screen;
    reverse_buffer = (unsigned char *)dictionary + total_codes * sizeof(decoder_entry_t);
    referenced = (unsigned char *)reverse_buffer + (total_codes - 256);

    for (i = 0; i < 256; ++i) {
        dictionary[i].prefix = NULL_CODE;
        dictionary[i].terminator = i;
    }

    while (1) {
        unsigned int code_bits = CODE_BITS(maxcode), code;
        unsigned int extras = (2 << code_bits) - maxcode - 1;

        do {
            if (src_index >= src_size)
                return -1;
            read_byte = src[src_index++];
            shifter |= read_byte << bits;
        } while ((bits += 8) < code_bits);

        code = shifter & ((1 << code_bits) - 1);
        shifter >>= code_bits;
        bits -= code_bits;

        if (code >= extras) {
            if (!bits) {
                if (src_index >= src_size)
                    return -1;
                shifter = src[src_index++];
                bits = 8;
            }
            code = (code << 1) - extras + (shifter & 1);
            shifter >>= 1;
            bits--;
        }

        if (code == maxcode)
            break;
        else if (code == CLEAR_CODE) {
            next_string = FIRST_STRING - 1;
            maxcode = FIRST_STRING;
            dictionary_full = 0;
        }
        else if (prefix == CLEAR_CODE) {
            *dst++ = code;
            next_string++;
            maxcode++;
        }
        else {
            unsigned int cti = (code == next_string) ? prefix : code;
            unsigned char *rbp = reverse_buffer, c;

            do {
                *rbp++ = dictionary[cti].terminator;
                if (rbp == reverse_buffer + total_codes - 256)
                    return -1;
            } while ((cti = dictionary[cti].prefix) != NULL_CODE);

            c = *--rbp;

            do
                *dst++ = *rbp;
            while (rbp-- != reverse_buffer);

            if (code == next_string)
                *dst++ = c;

            if (next_string >= FIRST_STRING && next_string < total_codes) {
                if (referenced[prefix >> 3] & (1 << (prefix & 7)))
                    dictionary[prefix].extra_references++;
                else
                    referenced[prefix >> 3] |= 1 << (prefix & 7);

                dictionary[next_string].prefix = prefix;
                dictionary[next_string].terminator = c;
                dictionary[next_string].extra_references = 0;
                referenced[next_string >> 3] &= ~(1 << (next_string & 7));
            }

            if (!dictionary_full) {
                maxcode++;
                if (++next_string > max_available_code) {
                    dictionary_full = 1;
                    maxcode--;
                }
            }

            if (dictionary_full) {
                for (next_string++; next_string <= max_available_code || (next_string = FIRST_STRING); next_string++)
                    if (!(referenced[next_string >> 3] & (1 << (next_string & 7))))
                        break;

                if (dictionary[dictionary[next_string].prefix].extra_references)
                    dictionary[dictionary[next_string].prefix].extra_references--;
                else
                    referenced[dictionary[next_string].prefix >> 3] &= ~(1 << (dictionary[next_string].prefix & 7));
            }
        }

        prefix = code;
    }
//emu_printf("output size %d\n",dst - original_dst);	
    return dst - original_dst;
}

#ifdef __cplusplus
}
#endif