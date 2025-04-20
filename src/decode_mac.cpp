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

static uint8_t* allocate_memory(const char* name, uint32_t alignedSize) {
    uint8_t* dst;

    if (name[0] == 'i' && name[1] == 'n' && name[2] == 't' && name[3] == 'r' && name[4] == 'o') {
        dst = (uint8_t*)current_lwram;
        current_lwram += alignedSize;
    }
    else if (name[0] == 'l' && name[1] == 'o' && name[2] == 'g' && name[3] == 'o') {
        dst = (uint8_t*)current_lwram;
        current_lwram += alignedSize;
    }
    else if (name[0] == 'e' && name[1] == 's' && name[2] == 'p' && name[3] == 'i' && name[4] == 'o') {
        dst = (uint8_t*)current_lwram;
        current_lwram += alignedSize;
    }
    else if (strstr(name, "Junky") || strstr(name, "Alien") || strstr(name, "Replicant")) {
        dst = (uint8_t*)current_lwram;
        current_lwram += 4;
    }
    else if (strstr(name, "Room") || strstr(name, "Font")) {
        dst = (uint8_t*)hwram_screen;
    }
    else if (strstr(name, "Title 6")) {
        GFS_Load(GFS_NameToId((int8_t*)"CONTROLS.BIN"), 0, (void*)(current_lwram + 36352), 147456);
        dst = (uint8_t*)NULL;
    }
    else if (((int)hwram_ptr) + alignedSize < end1) {
        dst = (uint8_t*)hwram_ptr;
        hwram_ptr += alignedSize;
    }
    else {
        dst = (uint8_t*)current_lwram;
        current_lwram += alignedSize;
    }

    return dst;
}

uint8_t* decodeLzss(File& f, const char* name, uint32_t& decodedSize) {
    // Read decodedSize as 4 bytes (big-endian)
    decodedSize = f.readUint32BE();
    uint32_t alignedSize = SAT_ALIGN(decodedSize);

    // Allocate memory
    uint8_t* dst = allocate_memory(name, alignedSize);
    if (!dst) return NULL; // Handle special case (e.g., "Title 6")

    uint8_t* const end = dst + decodedSize;
    uint8_t* cur = dst;

    while (cur < end) {
        uint8_t code = f.readByte();
        for (int i = 0; i < 8 && cur < end; ++i) {
            if ((code & 1) == 0) {
                *cur++ = f.readByte();
            } else {
                uint16_t offset = f.readUint16BE();
                const int len = (offset >> 12) + 3;
                offset &= 0xFFF;
				uint8_t* src = cur - offset - 1;
//				emu_printf("name %s len %d\n",name, len);
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
//	emu_printf("inf %d sup %d\n",a,b);
//	emu_printf("dst %p size %d\n",dst,decodedSize);

	return dst;
}

inline void setPixeli(int x, int y, uint8_t color, DecodeBuffer *buf) {
	y += buf->dst_y;
	x += buf->dst_x;
	const int offset = (y * 512) + x;
	buf->ptr[offset] = color;
}

#define CS1(x)                  (0x24000000UL + (x))

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf, unsigned char mask) {
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;
    static unsigned char remap[256];
    static bool remap_initialized = false;

    // Remap initialization (potential one-time setup)
    if (!remap_initialized) {
memset(remap, 0, sizeof(remap));
const unsigned char remap_values[] = {14, 15, 30, 31, 46, 47, 62, 63, 78, 79, 94, 95, 110, 111, 142, 143,
                                      126, 127, 254, 255, 174, 175, 190, 191, 206, 207, 222, 223, 238, 239};
// Use 16-bit copy where possible
		uint16_t* remap16 = reinterpret_cast<uint16_t*>(remap + 128);
		const uint16_t* src16 = reinterpret_cast<const uint16_t*>(remap_values);

		size_t i = 0;
		for (; i + 1 < sizeof(remap_values); i += 2) {
			*remap16++ = *src16++;
		}
        remap[14] = 128; remap[15] = 129; remap[30] = 130; remap[31] = 131;
        remap[160] = 14; remap[161] = 15; remap[190] = 150; remap[191] = 151;
        remap_initialized = true;
    }
    unsigned short cursor = 0;
    short bits = 1;
    uint8_t count = 0;
    unsigned short offset = 0;
    static uint8_t window[(1 << kBits)] __attribute__ ((aligned (4)));
//	uint8_t *window = (uint8_t *)(((uintptr_t)hwram_screen+ 50031) & ~31);

//	slTVOff(); on le fait bien avant
	w*=8;
	h/=8;

	for (unsigned short y = 0; y < h; ++y) {
		for (int x = 0; x < w; x++) {
			if (count == 0) {
				// Revert to original bitstream parsing
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

				// Optimized copy with 8-byte unrolling
				int i = 0;

				for (; i < count - 8; i += 8) {
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

				cursor += count;
				cursor &= kMask;
				x += count - 1; // Restore original x increment
				count = 0;
				continue;
			}

			window[cursor++] = window[offset++];
			cursor &= kMask;
			offset &= kMask;
			count--;
		}
		DMA_ScuMemCopy(buf->ptr, window, w);
		buf->ptr += w;
	}

	slTVOn();
	frame_y = frame_x = 0;
	frame_z = 30;
}

void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//emu_printf("decodeC211 src strt %p w %d h %d\n",src, w, h);
    const uint8_t *ptrs[4];      // Reduced stack size to fit in registers/cache
    uint16_t counts[8];
    uint8_t *dst = buf->ptr;
    uint16_t x = 0, y = 0;       // 16-bit to match typical SH-2 data sizes
    uint16_t sp = 0;             // Use uint16_t for consistency

    uint16_t row_offset = y * (buf->dst_h >> (buf->type == 1)); // Precompute row offset (4bpp: >>1, 8bpp: >>0)

    while (1) {
        uint8_t code = *src++;
        if (code & 0x80) {
            y++;
            x = 0;
            row_offset = y * (buf->dst_h >> (buf->type == 1)); // Update row offset
        }

        uint16_t count = code & 0x1F;
        if (count == 0) {
            count = (src[0] << 8) | src[1]; // Inline READ_BE_UINT16
            src += 2;
        }

        uint8_t op_type = code & 0x60;
        switch (op_type) {
        case 0x00: // Repeat
            if (count == 1) {
                if (sp == 0) return; // Early exit
                if (--counts[sp - 1] != 0) {
                    src = ptrs[sp - 1];
                } else {
                    sp--;
                }
            } else {
                if (sp >= 4) return; // Bounds check
                ptrs[sp] = src;
                counts[sp] = count;
                sp++;
            }
            break;

        case 0x20: // Skip
            x += count;
            break;

        case 0x40: // Fill
            if (count == 1) return; // Early exit
            {
                uint8_t color = *src++;
                if (buf->type == 1) { // 4bpp
                    color &= 0x0F;
                    uint16_t offset = row_offset + (x >> 1);
                    uint8_t odd = x & 1;
                    for (uint16_t i = 0; i < count; i++) {
                        uint8_t cur_x = x + i;
                        dst[offset] = odd ? (dst[offset] | color) : (color << 4);
                        offset += odd;
                        odd = !odd;
                    }
				}
				else if (buf->type == 2) { // 8bpp menu
//					memset(dst + x + buf->dst_x + ((y+ buf->dst_y) * 512), *src++, count);
                    for (uint16_t i = 0; i < count; i++) {
                        setPixeli(x + i, y, color, buf);
                    }
                } else { // 8bpp
                    uint8_t *target = dst + row_offset + x;
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
				uint16_t offset = row_offset + (x >> 1);
				uint8_t odd = x & 1;
				uint16_t i;
				for (i = 0; i + 1 < count; i += 2) {
					uint8_t color1 = *src++ & 0x0F;
					uint8_t color2 = *src++ & 0x0F;
					dst[offset] = (color1 << 4) | color2;
					offset++;
				}
				if (i < count) {
					uint8_t color = *src++ & 0x0F;
					dst[offset] = odd ? (dst[offset] | color) : (color << 4);
				}
			} else if (buf->type == 2) { // 8bpp menu
				for (uint16_t i = 0; i < count; i++) {
					setPixeli(x + i, y, *src++, buf);
				}
			}
			else { // 8bpp
				uint8_t *target = dst + row_offset + x;
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
