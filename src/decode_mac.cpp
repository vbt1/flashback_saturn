//#pragma GCC optimize ("O2")
//#include <assert.h>
extern "C" {
#include <string.h>
#include <strings.h>

#include <ctype.h>
#include "sat_mem_checker.h"
#include <sl_def.h>
}

#include "decode_mac.h"
#include "util.h"
#include "saturn_print.h"

extern Uint8 *hwram;
extern Uint8 *hwram_ptr;
extern Uint8 *hwram_screen;
extern Uint8 *save_lwram;
extern Uint8 *current_lwram;
extern unsigned int end1;
Uint8 *current_dram=(Uint8 *)0x22440000;
Uint8 *current_dram2=(Uint8 *)0x22600000;

uint8_t *decodeLzss(File &f,const char *name, const uint8_t *_scratchBuffer, uint32_t &decodedSize) {

emu_printf("lzss %s %05d\n", name, decodedSize);
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
		{
			//emu_printf("hwram1 %d %s\n", decodedSize, name);
			dst = (uint8_t *)hwram_screen;
		}
		else
		{
			if(strstr(name,"polygons")   != NULL || strstr(name,"movie") != NULL)
			{
				dst = (uint8_t *)current_lwram;
				current_lwram += SAT_ALIGN(decodedSize);
			}
			else
			{
				if ((int)hwram_ptr+decodedSize<=end1)
				{
	//				emu_printf("hwram2 %d %s\n", decodedSize, name);
					dst = (uint8_t *)hwram_ptr;
					hwram_ptr += SAT_ALIGN(decodedSize);
				}
				else
				{
	//				emu_printf("lwram_new %d %s\n", decodedSize, name);
					dst = (uint8_t *)current_lwram;
					current_lwram += SAT_ALIGN(decodedSize);
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
	/*
	if(count != decodedSize)  // vbt ne pas toucher
	{
//		emu_printf("count != decodedSize  %d %d\n", count, decodedSize);
		return dst;		
	}*/
	return dst;
}

static void setPixel(int x, int y, int w, int h, uint8_t color, DecodeBuffer *buf) {
	y += buf->y;
	if (y >= 0 && y < buf->h) {
//	buf->setPixel(buf, x, y, color);
/*
		if (buf->xflip) {
			x = w - 1 - x;
		}*/
		x += buf->x;
		if (x >= 0 && x < buf->w) 
		{
			buf->setPixel(buf, x, y, color);
		}
	}
}

/*
	const int offset = (y-buf->y) * (buf->h2>>1) + ((x>>1)-(buf->x>>1));	
	if(x&1)
		buf->ptrsp[offset] |= (color&0x0f);
	else
	{
		buf->ptrsp[offset] = ((color&0x0f)<<4);
	}
*/


#define CS1(x)                  (0x24000000UL + (x))

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf, unsigned char mask) {
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;
	static const unsigned char lut[32]={14,15,30,31,46,47,62,63,78,79,94,95,110,111,142,143,126,127,
										158,159,174,175,190,191,206,207,222,223,238,239,254,255};
    unsigned short cursor = 0;
    short bits = 1;
    uint8_t count = 0;
    unsigned short offset = 0;
    static uint8_t window[(3 << kBits)] __attribute__ ((aligned (4)));
    uint8_t *tmp_ptr = (uint8_t *)window + 4096;

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
//                    const uint8_t color = (*src++) &0x8f;
//                    uint8_t color = (*src++) & mask;
                    uint8_t color;
					if(*src>=128)
					color = (*src++) & mask;
					else
					color = (*src++);

					if(mask!=0xff)
					{
						if(color >= 128 && color < 160)
						{
							color = lut[color&0x1f];
						}
						else
						{
							if(color==14)	color=128;
							if(color==15)	color=129;
							if(color==30)	color=130;
							if(color==31)	color=131;
						}
					}
                    window[cursor++] = color;
                    *tmp_ptr++ = color;
/*	if(x&1)
	{
		*tmp_ptr |= (color&0x0f);
		tmp_ptr++;
	}
	else
		*tmp_ptr = ((color&0x0f)<<4);*/
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
/*
	if(x&1)
	{
		*tmp_ptr |= (color&0x0f);
		tmp_ptr++;
	}
	else
	{
		*tmp_ptr = ((color&0x0f)<<4);
	}
*/
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
					setPixel(x++, y, w, h, color, buf);
				}
			} else {
				for (int i = 0; i < count; ++i) {
					setPixel(x++, y, w, h, *src++, buf);
				}
			}
		}
	}
//emu_printf("decodeC211 end\n");
	
}