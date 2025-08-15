#pragma GCC optimize ("O2")
//#include <assert.h>
extern "C" {
#include <string.h>
#include <strings.h>

#include <ctype.h>
#include "sat_mem_checker.h"
#include <sl_def.h>
#include <sega_gfs.h>
extern Uint8 *hwram;
extern Uint8 *hwram_ptr;
extern Uint8 *hwram_screen;
extern Uint8 *current_lwram;
extern unsigned int end1;
extern Uint8 frame_x;
extern Uint8 frame_y;
extern Uint8 frame_z;
}


#include "decode_mac.h"
#include "util.h"
#include "saturn_print.h"

static uint8_t* allocate_memory(const uint8_t type, const uint16_t id, uint32_t alignedSize) {
//	emu_printf("allocate_memory %d %04x\n", id, alignedSize);
    uint8_t* dst;

    // Fast path for type 13 or 14
    if (type == 13 || type == 14) {
        dst = current_lwram;
        current_lwram += alignedSize;
        return dst;
    }

    if (type == 31) //  demo
		return (uint8_t *)SCRATCH;


    // Fast path for specific IDs
    if (id == 3100 || id == 3300 || id == 3400) {
        dst = current_lwram;
        current_lwram += 4;
        return dst;
    }

    // Fast path for type 12 with ID range or ID 4000
    if ((type == 12 && (id - 1000) <= (1461 - 1000)) || id == 4000) {
        return hwram_screen;
    }

    // Special case for ID 5500
    if (id == 5500) {
        GFS_Load(GFS_NameToId((int8_t*)"CONTROLS.BIN"), 0, (void*)(current_lwram + 36352), 147456);
		memset(current_lwram + 183808,0x00,45568);
        return NULL;
    }

    // Default case: Use hwram_ptr if possible
    if ((int)hwram_ptr + alignedSize < end1) {
        dst = hwram_ptr;
        hwram_ptr += alignedSize;
        return dst;
    }

    // Fallback to lwram
//    emu_printf("lw %d id %d sz %d %x %x\n", type, id, alignedSize, ((int)hwram_ptr) + alignedSize, end1);

    dst = current_lwram;
    current_lwram += alignedSize;
    return dst;
}

uint8_t* decodeLzss(File& f, uint32_t& decodedSize, const ResourceMacEntry *entry) {
//uint8_t* decodeLzss(File& f, const ResourceMacEntry *entry) {
	uint32_t srcSize = entry->compressedSize;
    decodedSize = entry->size;
	f.batchSeek(8);
//	emu_printf("Lzss sz2 %d %d\n", srcSize, decodedSize);
    uint32_t alignedSize = SAT_ALIGN(decodedSize);

    // Allocate memory
//    uint8_t* dst = allocate_memory(type, id, alignedSize);
    uint8_t* dst = allocate_memory(entry->type, entry->id, alignedSize);
    if (!dst) return NULL; // Handle special case (e.g., "Title 6")

	uint8_t* batch = (uint8_t*)SCRATCH+4096;
	if(decodedSize>300000)
		batch = (uint8_t*)FRONT;

	batch = f.batchRead(batch, srcSize);

    uint8_t* const end = dst + decodedSize;
    uint8_t* cur = dst;

   while (cur < end) {
        uint8_t code = *batch++;
        for (int i = 0; i < 8 && cur < end; ++i) {
            if ((code & 1) == 0) {
                *cur++ = *batch++;
            } else {
				uint16_t offset = READ_BE_UINT16(batch);
				batch += 2;

                const int len = (offset >> 12) + 3;
                offset &= 0xFFF;
				uint8_t* src = cur - offset - 1;
				cur[0] = src[0];
				cur[1] = src[1];
				cur[2] = src[2];

				// Handle remaining bytes
				for (unsigned char i =3; i < len; i++) {
					cur[i] = src[i];
				}
				cur+=len;
            }
            code >>= 1;
        }
    }
	return dst;
}

#define CS1(x)                  (0x24000000UL + (x))

static unsigned char remap[256] __attribute__((aligned(4)));
 //   static bool remap_initialized = false;
void remap_colors()
{
    // Remap initialization (potential one-time setup)
//    if (!remap_initialized) {
        memset(remap, 0, sizeof(remap));
        const unsigned char remap_values[] = {14, 15, 30, 31, 46, 47, 62, 63, 78, 79, 94, 95, 110, 111, 142, 143,
                                              126, 127, 254, 255, 174, 175, 190, 191, 206, 207, 222, 223, 238, 239};
        uint16_t* remap16 = (uint16_t*)(remap + 128);
        const uint16_t* src16 = (const uint16_t*)remap_values;

        for (size_t i = 0; i + 1 < sizeof(remap_values); i += 2) {
            *remap16++ = *src16++;
        }
        remap[14] = 128; remap[15] = 129; remap[30] = 130; remap[31] = 131;
        remap[160] = 14; remap[161] = 15; remap[190] = 150; remap[191] = 151;
//        remap_initialized = true;
//    }
}

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf, unsigned char mask) {
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;

    unsigned short cursor = 0;
    short bits = 1;
    uint8_t count = 0;
    unsigned short offset = 0;
    static uint8_t window[(1 << kBits)] __attribute__ ((aligned (4)));

    w *= 8;
    h /= 8;

    for (unsigned short y = 0; y < h; ++y) {
        for (int x = 0; x < w; x++) {
            if (count == 0) {
                uint8_t carry = bits & 1;
                bits >>= 1;
                if (bits == 0) {
                    bits = *src++;
                    if (carry) bits |= 0x100;
                    carry = bits & 1;
                    bits >>= 1;
                }

                if (!carry) {
                    uint8_t color = *src++;
                    if (mask != 0xFF) {
                        color = remap[color] ? remap[color] : color;
                    }
                    window[cursor++] = color;
                    cursor &= kMask;
                    continue;
                }

                offset = ((unsigned short)src[0] << 8) | src[1]; // Big-endian read
                src += 2;
                count = 3 + (offset >> 12);
                offset = (cursor - (offset & kMask) - 1) & kMask;
            }

            if (cursor <= 4077 && offset <= 4077) {
                uint8_t *dst = window + cursor;
                const uint8_t *src_win = window + offset;

                // Check if the memory addresses are aligned
                if (((uintptr_t)dst & 3) == 0 && ((uintptr_t)src_win & 3) == 0) {
                    // Use 32-bit loads and stores for aligned memory with loop unrolling
                    int i = 0;
                    for (; i < count - 7; i += 8) {
                        *((uint32_t*)(dst + i)) = *((uint32_t*)(src_win + i));
                        *((uint32_t*)(dst + i + 4)) = *((uint32_t*)(src_win + i + 4));
                    }
                    for (; i < count; i++) {
                        dst[i] = src_win[i];
                    }
                } else {
                    // Byte-by-byte copy for unaligned memory with loop unrolling
                    int i = 0;
                    for (; i < count - 7; i += 8) {
                        dst[i] = src_win[i];
                        dst[i + 1] = src_win[i + 1];
                        dst[i + 2] = src_win[i + 2];
                        dst[i + 3] = src_win[i + 3];
                        dst[i + 4] = src_win[i + 4];
                        dst[i + 5] = src_win[i + 5];
                        dst[i + 6] = src_win[i + 6];
                        dst[i + 7] = src_win[i + 7];
                    }
                    for (; i < count; i++) {
                        dst[i] = src_win[i];
                    }
                }

                cursor += count;
                cursor &= kMask;
                x += count - 1;
                count = 0;
                continue;
            }

            window[cursor++] = window[offset++];
            cursor &= kMask;
            offset &= kMask;
            count--;
        }
//        DMA_ScuMemCopy(buf->ptr, window, w);
// vbt : ne jamais remettre de dma, memcpyl : 765ms, dma : 765ms + plantages aléatoires
		memcpyl(buf->ptr, window, w);
//		emu_printf("DMA_ScuMemCopy %p %p w %d\n", buf->ptr, window, w);
        buf->ptr += w;
//		SCU_DMAWait();
    }

    slTVOn();
    frame_y = frame_x = 0;
    frame_z = 30;
}

void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
    const uint8_t *ptrs[4];
    uint16_t counts[4];
    uint8_t *dst = buf->ptr;
    uint16_t x = 0, y = 0, sp = 0;
    uint32_t row_offset = 0;

    while (1) {
        uint8_t code = *src++;
        if (code & 0x80) {
            y++;
            x = 0;
            // Stride: buf->dst_h >> 1 for 4bpp, buf->dst_h for type == 0, buf->dst_w for type == 2
            row_offset = (buf->type == 1 ? y * (buf->dst_h >> 1) : 
                         (buf->type == 2 ? (y + buf->dst_y) * buf->dst_w : y * buf->dst_h));
        }

        uint16_t count = code & 0x1F;
        if (count == 0) {
            count = (src[0] << 8) | src[1];
            src += 2;
        }

        switch (code & 0x60) {
        case 0x00: // Repeat
            if (count == 1) {
                if (sp == 0) return;
                if (--counts[sp - 1] != 0) {
                    src = ptrs[sp - 1];
                } else {
                    sp--;
                }
            } else {
                if (sp >= 4) return;
                ptrs[sp] = src;
                counts[sp] = count;
                sp++;
            }
            break;

        case 0x20: // Skip
            x += count;
            break;

        case 0x40: // Fill
            if (count == 1) return;
            {
                uint8_t color = *src++;
                if (buf->type == 1) { // 4bpp
                    uint8_t *target = dst + row_offset + (x >> 1);
                    uint8_t odd = x & 1;
                    color &= 0x0F;
                    for (uint16_t i = 0; i < count; i++) {
                        *target = odd ? (*target | color) : (color << 4);
                        target += odd;
                        odd = !odd;
                    }
                } else { // 8bpp font or menu
                    uint8_t *target = dst + row_offset + x + (buf->type == 2 ? buf->dst_x : 0);
                    uint16_t i;
                    for (i = 0; i + 3 < count; i += 4) {
                        target[i] = color;
                        target[i + 1] = color;
                        target[i + 2] = color;
                        target[i + 3] = color;
                    }
                    for (; i < count; i++) {
                        target[i] = color;
                    }
                }
                x += count;
            }
            break;

        case 0x60: // Copy
            if (buf->type == 1) { // 4bpp
                uint8_t *target = dst + row_offset + (x >> 1);
                uint8_t odd = x & 1;
                uint16_t i;
                for (i = 0; i + 1 < count; i += 2) {
                    uint8_t color1 = *src++ & 0x0F;
                    uint8_t color2 = *src++ & 0x0F;
                    *target++ = (color1 << 4) | color2;
                }
                if (i < count) {
                    uint8_t color = *src++ & 0x0F;
                    *target = odd ? (*target | color) : (color << 4);
                }
            } else { // 8bpp font or menu
                uint8_t *target = dst + row_offset + x + (buf->type == 2 ? buf->dst_x : 0);
                uint16_t i;
                for (i = 0; i + 3 < count; i += 4) {
                    target[i] = *src++;
                    target[i + 1] = *src++;
                    target[i + 2] = *src++;
                    target[i + 3] = *src++;
                }
                for (; i < count; i++) {
                    target[i] = *src++;
                }
            }
            x += count;
            break;
        }
    }
}