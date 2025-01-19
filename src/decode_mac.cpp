#pragma GCC optimize ("O2")
//#include <assert.h>
extern "C" {
#include <string.h>
#include <strings.h>

#include <ctype.h>
#include "sat_mem_checker.h"
#include <sl_def.h>

extern Uint8 *hwram;
extern Uint8 *hwram_ptr;
extern Uint8 *hwram_screen;
extern Uint8 *current_lwram;
extern unsigned int end1;
}

#include "decode_mac.h"
#include "util.h"
#include "saturn_print.h"

uint8_t *decodeLzss(File &f,const char *name, const uint8_t *_scratchBuffer, uint32_t &decodedSize) {

//emu_printf("lzss %s %05d\n", name, decodedSize);
   // Read the decoded size
    decodedSize = f.readUint32BE();
    uint32_t alignedSize = SAT_ALIGN(decodedSize);

    // Pointer for memory allocation
    uint8_t *dst;

    // Cache strstr results
    bool isJunky = strstr(name, "Junky") || strstr(name, "Alien") || strstr(name, "Replicant");
    bool isRoom = strstr(name, "Room") || strstr(name, "Title 6");

    // Memory allocation logic
	if(strncmp("intro", name, 5) == 0 
		|| strncmp("logo", name, 4) == 0 
		|| strncmp("espion", name, 5) == 0)
		{
        dst = (uint8_t *)current_lwram;
        current_lwram += alignedSize;
		}

    else if (isJunky) {
        // Special case for "Junky"
        dst = (uint8_t *)current_lwram;
        current_lwram += 4;
    } else if (isRoom) {
        // Allocates from hwram_screen
        dst = (uint8_t *)hwram_screen;
    } else if (((int)hwram_ptr) + alignedSize < end1) {
        // Allocates from hwram_ptr if there's enough space
        dst = (uint8_t *)hwram_ptr;
        hwram_ptr += alignedSize;
    } else {
        // Fallback to current_lwram
        dst = (uint8_t *)current_lwram;
        current_lwram += alignedSize;
    }

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
                for (int j = 0; j < len; ++j) {
                    cur[j] = src[j];
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
	buf->setPixel(buf, x, y, color);
}

#define CS1(x)                  (0x24000000UL + (x))

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf, unsigned char mask) {
/*
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;
    static unsigned char remap[256];
    static bool remap_initialized = false;
    if (!remap_initialized) {
        memset(remap, 0, sizeof(remap));
        for (unsigned char i = 0; i < 30; ++i) {
            remap[128 + i] = (unsigned char[]){14, 15, 30, 31, 46, 47, 62, 63, 78, 79, 94, 95, 110, 111, 142, 143,
                                              126, 127, 254, 255, 174, 175, 190, 191, 206, 207, 222, 223, 238, 239}[i];
        }
        remap[14] = 128; remap[15] = 129; remap[30] = 130; remap[31] = 131;
        remap[160] = 14; remap[161] = 15; remap[190] = 150; remap[191] = 151;
        remap_initialized = true;
    }
*/
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

//	slTVOff(); on le fait bien avant
	w*=8;
    for (unsigned short y = 0; y < h/8; ++y) 
	{
		for (int x = 0; x < w; x++) 
		{
            if (count == 0) {
                uint8_t carry = bits & 1;
                bits >>= 1;
                if (bits == 0) {
                    bits = *src++;
                    if (carry) {
                        bits |= 0x100;
                    }
                    carry = bits & 1;
                    bits >>= 1;
                }

                if (!carry) {
                    uint8_t color = *src++;

					if(mask!=0xff) // cas ecran niveau
					{
                        color = remap[color] ? remap[color] : color;
					}

                    window[cursor++] = color;
                    cursor &= kMask;
                    continue;
                }
                offset = READ_BE_UINT16(src); src += 2;
                count = 3 + (offset >> 12);
 //               offset &= kMask;
                offset = (cursor - offset - 1) & kMask;
            }
//------------------------
//			unsigned short max_pos = (cursor > offset) ? cursor : offset;
//			unsigned short max_pos = (cursor > offset) ? cursor : offset;
//			if (cursor+count <=kMask && offset+count <=kMask)
//			if(max_pos + count <= kMask)
			if(cursor <= 4077 && offset <= 4077)
//		if(max_pos <= 4077)
			{
				uint8_t *dst = &window[cursor];
				uint8_t *src = &window[offset];

				for (size_t i = 0; i < count; ++i) {
					dst[i] = src[i];
				}
				cursor += count;
//				cursor &= kMask;
				x += count-1;
				count=0;
				continue;
			}
//------------------------
			window[cursor++] = window[offset++];
			cursor &= kMask;
			offset &= kMask;
			--count;
		}
		DMA_ScuMemCopy(buf->ptr, window, w);
		buf->ptr += w;
    }
	slTVOn();
}

void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//emu_printf("decodeC211 src strt %p w %d h %d\n",src, w, h);
	struct {
		const uint8_t *ptr;
		int repeatCount;
	} stack[512];
	int y = 0;
	int x = 0;
	int sp = 0;

//	while (1) {
//    const uint8_t *srcEnd = src + w * h;  // Calculate end of the source buffer
//    while (src < srcEnd) {
    while (1) {
		const uint8_t code = *src++;
        if (code & 0x80) {
			++y;
			x = 0;
		}
	
		int count = code & 0x1F;
		if (count == 0) {
			count = READ_BE_UINT16(src); src += 2;
//			count = ((uint16_t*)src)[0]; src += 2;
		}
        if (!(code & 0x40)) {
            if (!(code & 0x20)) {
				if (count == 1) {
//					assert(sp > 0);
					if(sp <= 0)
						break;
					--stack[sp - 1].repeatCount;
					if (stack[sp - 1].repeatCount >= 0) {
						src = stack[sp - 1].ptr;
					} else {
						--sp;
					}
				} else {
//					assert(sp < ARRAYSIZE(stack));
					if(sp >= ARRAYSIZE(stack))
						break;
					stack[sp].ptr = src;
					stack[sp].repeatCount = count - 1;
					++sp;
				}
			} else {
				x += count;
			}
		} else {
            if (!(code & 0x20)) {
				if (count == 1) {
					return;
				}
				uint8_t color = *src++;
				int offset = 0;
                switch(buf->type)
                {
                    case 0: // spc
                        offset = y * buf->h + x;
                        memset(&buf->ptr[offset],color,count);
                        x += count;
                        break;

					case 1: //perso 4bpp & ennemis
						if(!x&1)
						{
							offset = y * (buf->h>>1) + (x>>1);
							memset(&buf->ptr[offset],(color&0x0f)|color<<4,((count)>>1));
							if(count&1)
								buf->ptr[offset+(count>>1)]=((color&0x0f)<<4);
							x+=count;
							break;
						}
                    default: // font 8bpp et menu inventaire

						for (int i = 0; i < count; ++i) {
							setPixeli(x++, y, color, buf);
						}
                        break;
                }
			} else {
				for (int i = 0; i < count; ++i) {
					setPixeli(x++, y, *src++, buf);
				}
			}
		}
	}
//emu_printf("decodeC211 end\n");
}
