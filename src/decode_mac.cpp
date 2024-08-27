//#pragma GCC optimize ("O2")
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
//Uint8 *current_dram=(Uint8 *)0x22440000;
Uint8 *current_dram2=(Uint8 *)0x22600000;
}

#include "decode_mac.h"
#include "util.h"
#include "saturn_print.h"



uint8_t *decodeLzss(File &f,const char *name, const uint8_t *_scratchBuffer, uint32_t &decodedSize) {

//emu_printf("lzss %s %05d\n", name, decodedSize);
	decodedSize = f.readUint32BE();
	
	uint8_t *dst;
	 if(strstr(name,"Junky") != NULL
	 )
	 {
		dst = (uint8_t *)current_lwram;
		current_lwram+=4;
	 }
	 else if(strstr(name,"Replicant") != NULL
	 || strstr(name,"Mercenary") != NULL
	 || strstr(name,"Alien") != NULL
//	 || strstr(name,"Objects") != NULL
	 )
	 {
		dst = (uint8_t *)current_dram2;
		current_dram2 += SAT_ALIGN(decodedSize);	 
	 }
	else
	{
		if(strstr(name,"Room") != NULL)
//		if(strstr(name,"Room") != NULL || strstr(name,"polygons")   != NULL)
		{
			//emu_printf("hwram1 %d %s\n", decodedSize, name);
			dst = (uint8_t *)hwram_screen;
		}
		else
		{
			if(strstr(name,"polygons")   != NULL || strstr(name,"movie") != NULL)
//			if(strstr(name,"movie") != NULL)
			{
				dst = (uint8_t *)current_lwram;
				current_lwram += SAT_ALIGN(decodedSize);
			}
			else
			{
				if (((int)hwram_ptr)+SAT_ALIGN(decodedSize)<end1)
				{
//					emu_printf("hwram2 %d %s\n", decodedSize, name);
					dst = (uint8_t *)hwram_ptr;
					hwram_ptr += SAT_ALIGN(decodedSize);
				}
				else
				{
					dst = (uint8_t *)current_lwram;
					current_lwram += SAT_ALIGN(decodedSize);
//					emu_printf("lwram_new %d %p %s\n", decodedSize, current_lwram, name);
				}
			}
		}
	}
	uint32_t count = 0;

	while (count < decodedSize) {
		const int code = f.readByte();
		for (int i = 0; i < 8 && count < decodedSize; ++i) {
			if ((code & (1 << i)) == 0) {
				dst[count++] = f.readByte();
			} else {
				int offset = f.readUint16BE();
				const int len = (offset >> 12) + 3;
				offset &= 0xFFF;

				for (int j = 0; j < len; ++j) {
					dst[count + j] = dst[count - offset - 1 + j];
				}
				count += len;
			}
		}
	}
//	emu_printf("inf %d sup %d\n",a,b);
//	emu_printf("dst %p\n",dst);

	return dst;
}

inline void setPixeli(int x, int y, uint8_t color, DecodeBuffer *buf) {
	y += buf->y;
	x += buf->x;
	buf->setPixel(buf, x, y, color);
}

#define CS1(x)                  (0x24000000UL + (x))

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf, unsigned char mask) {
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;

	static const unsigned char lut[30]={14,15,30,31,46,47,62,63,78,79,94,95,110,111,142,143,
										126,127,254,255,174,175,190,191,206,207,222,223,238,239};
    unsigned short cursor = 0;
    short bits = 1;
    uint8_t count = 0;
    unsigned short offset = 0;
    static uint8_t window[(3 << kBits)] __attribute__ ((aligned (4)));
    uint8_t *tmp_ptr = (uint8_t *)window + 4096;

	slTVOff();

    for (unsigned short y = 0; y < h; ++y) {
        unsigned short x = 0;

        while (x < w) {
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

					if(mask!=0xff)
					{
						if(color >= 128 && color < 158)
						{
							color = lut[color&0x1f];
						}
						else
						{
						   switch (color) 
						   {
								case 14:  color = 128; break;
								case 15:  color = 129; break;
								case 30:  color = 130; break;
								case 31:  color = 131; break;
								case 160: color = 14;  break;  // sans masque le masque
								case 161: color = 15;  break;
								case 190: color = 150; break;  // si on enleve le masque
								case 191: color = 151; break;
							}
						}
					}

                    window[cursor++] = color;
                    *tmp_ptr++ = color;

                    cursor &= kMask;
                    x++;
                    continue;
                }
                offset = READ_BE_UINT16(src); src += 2;
                count = 3 + (offset >> 12);
                offset &= kMask;
                offset = (cursor - offset - 1) & kMask;
            }

            const short tt = kMask - count;
			{
                while (count > 0 && x < w) {
                    uint8_t color = window[offset++];
                    window[cursor++] = color;
                    *tmp_ptr++ = color;

                    cursor &= kMask;
                    offset &= kMask;
                    count--;
                    x++;
                }
            }
        }

        if ((y & 7) == 7) {
            tmp_ptr = (uint8_t *)window + 4096;
            memcpyl(buf->ptr, tmp_ptr, w * 8);
            buf->ptr += w * 8;
        }
    }
	slTVOn();
}

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