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
extern Uint8 *save_lwram;
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
    bool isJunky = strstr(name, "Junky") != NULL || strstr(name, "Alien") != NULL || strstr(name, "Replicant") != NULL;
    bool isRoom = strstr(name, "Room") != NULL;

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

	uint32_t count = 0;

	while (count < decodedSize) {
		int code = f.readByte();
		for (int i = 0; i < 8 && count < decodedSize; ++i) {
//			if ((code & (1 << i)) == 0) {
			if ((code & 1) == 0) {
				dst[count++] = f.readByte();
			} else {
#if 1
				int offset = f.readUint16BE();
				const int len = (offset >> 12) + 3;
				offset &= 0xFFF;

				for (int j = 0; j < len; ++j) {
					dst[count + j] = dst[count - offset - 1 + j];
				}
				count += len;
#else
int off = f.readUint16BE();
int len = (off >> 12) + 3;
off &= 0xFFF;

if (len > 0) {
    int src = count - off - 1;
    
    // Optimize for common case of non-overlapping or simple copy
/*
    if (off + 1 >= len) {
        // Equivalent to memcpy, but potentially more efficient
//		memcpy(dst + count, dst + src, len);
				uint8_t *src1 = (uint8_t *)dst + src;
				uint8_t *dst1 = (uint8_t *)dst + count;

				for (size_t i = 0; i < len; ++i) {
					*dst1++ = *src1++;
				}


    } else {
        // Handle overlapping copies carefully
        for (int j = 0; j < len; ++j) {
            dst[count + j] = dst[src + j];
        }
    }
*/	
__asm__ volatile (
    // Initialize pointers and counters
    "mov %[dst], r0\n\t"       // r0 = destination pointer
    "add %[count], r0\n\t"     // Adjust r0 with count offset
    "mov r0, r1\n\t"           // r1 = adjusted destination pointer
    "sub %[off], r0\n\t"       // r0 = source pointer (destination - offset)
    "add #-1, r0\n\t"          // Adjust source pointer

    "mov %[len], r2\n\t"       // r2 = length (total bytes to copy)

    // Word copy loop (2 bytes at a time)
    "2:\n\t"
    "cmp/hs #2, r2\n\t"        // Check if 2 or more bytes remain
    "bt 3f\n\t"                // If less than 2 bytes, branch to byte copy
    "mov.w @r0+, r3\n\t"       // Load 2 bytes (word) from source
    "mov.w r3, @r1+\n\t"       // Store 2 bytes to destination
    "add #-2, r2\n\t"          // Decrement remaining length by 2
    "bra 2b\n\t"               // Repeat word copy loop
    "nop\n\t"                  // Delay slot

    // Byte copy loop
    "3:\n\t"
    "cmp/eq #0, r2\n\t"        // Check if any bytes remain
    "bt 4f\n\t"                // If none, exit
    "mov.b @r0+, r3\n\t"       // Load byte from source
    "mov.b r3, @r1+\n\t"       // Store byte to destination
    "add #-1, r2\n\t"          // Decrement remaining length by 1
    "bra 3b\n\t"               // Repeat byte copy loop
    "nop\n\t"                  // Delay slot

    // Exit
    "4:\n\t"
    :
    : [dst] "r" (dst), 
      [count] "r" (count), 
      [off] "r" (off), 
      [len] "r" (len)
    : "r0", "r1", "r2", "r3", "memory"
);


    count += len;
}
#endif				
			}
			code >>= 1;
		}
	}

//	emu_printf("inf %d sup %d\n",a,b);
//	emu_printf("dst %p\n",dst);

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
                offset &= kMask;
                offset = (cursor - offset - 1) & kMask;
            }
//------------------------
			if (cursor+count <=kMask && offset+count <=kMask)
			{
				uint8_t *dst = &window[cursor];
				uint8_t *src = &window[offset];

				for (size_t i = 0; i < count; ++i) {
					*dst++ = *src++;
				}
				cursor += count;
				cursor &= kMask;
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


#if 0
void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//emu_printf("decodeC211 src strt %p\n",src);
	struct {
		const uint8_t *ptr;
		int repeatCount;
	} stack[512];
	int y = 0;
	int x = 0;
	int sp = 0;

	while (1) {
		const uint8_t code = *src++;
		if ((code & 0x80) != 0) {
			++y;
			x = 0;
		}
	
		int count = code & 0x1F;
		if (count == 0) {
			count = READ_BE_UINT16(src); src += 2;
//			count = ((uint16_t*)src)[0]; src += 2;
		}
		if ((code & 0x40) == 0) {
			if ((code & 0x20) == 0) {
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
			if ((code & 0x20) == 0) {
				if (count == 1) {
					return;
				}
				const uint8_t color = *src++;
				for (int i = 0; i < count; ++i) {
					setPixeli(x++, y, color, buf);
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
#else
void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//emu_printf("decodeC211 src strt %p\n",src);
	struct {
		const uint8_t *ptr;
		int repeatCount;
	} stack[512];
	int y = 0;
	int x = 0;
	int sp = 0;

	while (1) {
		const uint8_t code = *src++;
		if ((code & 0x80) != 0) {
			++y;
			x = 0;
		}
	
		int count = code & 0x1F;
		if (count == 0) {
			count = READ_BE_UINT16(src); src += 2;
//			count = ((uint16_t*)src)[0]; src += 2;
		}
		if ((code & 0x40) == 0) {
			if ((code & 0x20) == 0) {
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
			if ((code & 0x20) == 0) {
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
                        x+=count;
                        break;

					case 1: //perso 4bpp & ennemis
						offset = y * (buf->h>>1) + (x>>1);

						if(x&1)
						{

							for (int i = 0; i < count; ++i) {
								setPixeli(x++, y, color, buf);
							}
							goto fin;
						}
						else
						{
							memset(&buf->ptr[offset],(color&0x0f)|color<<4,((count)>>1));
							if(count&1)
								buf->ptr[offset+(count>>1)]=((color&0x0f)<<4);
							x+=count;
						}
fin:
                        break;
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
#endif