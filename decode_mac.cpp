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

uint8_t *decodeLzss(File &f,const char *name, const uint8_t *_scratchBuffer, uint32_t &decodedSize) {

//emu_printf("lzss %s %05x\n", name, decodedSize);
	decodedSize = f.readUint32BE();
	
	uint8_t *dst;

	 if(strstr(name,"Junky") != NULL
	 || strstr(name,"Replicant") != NULL
	 || strstr(name,"Alien") != NULL
	 || strstr(name,"Mercenary") != NULL
	 )
	 {
		dst = (uint8_t *)current_lwram+(decodedSize);  // /2 car 4bpp
		current_lwram += SAT_ALIGN(4);
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
//				dst = (uint8_t *)sat_malloc(decodedSize);
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

//				const unsigned int target = count - offset - 1;
				/*
				if(-offset+len<0 && len >8)
					memcpy(&dst[count],&dst[count - offset - 1],len);
				else*/
				{
					for (int j = 0; j < len; ++j) {
						dst[count + j] = dst[count - offset - 1 + j];
					}
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
		if (buf->xflip) {
			x = w - 1 - x;
		}
		x += buf->x;
		if (x >= 0 && x < buf->w) {
			buf->setPixel(buf, x, y, color);
		}
	}
}
#define CS1(x)                  (0x24000000UL + (x))

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
    static const short kBits = 12;
    static const short kMask = (1 << kBits) - 1;

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
                    const uint8_t color = *src++;
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

            if (cursor <= tt && offset <= tt) {
//				emu_printf("data size count %d\n",count);
//				if(count>18)
//				emu_printf("count %d\n",count);
/*
                while (count >= 18 && x + 18 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 18;
                    x += 18;
                }
*/
                // Unrolled loop
                while (count >= 16 && x + 16 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 16;
                    x += 16;
                }

                while (count >= 12 && x + 12 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 10;
                    x += 10;
                }


                while (count >= 8 && x + 8 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 8;
                    x += 8;
                }
/*
                while (count >= 6 && x + 6 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 6;
                    x += 6;
                }
*/
                while (count >= 4 && x + 4 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 4;
                    x += 4;
                }

                while (count >= 2 && x + 2 <= w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count -= 2;
                    x += 2;
                }

                while (count > 0 && x < w) {
                    *tmp_ptr++ = window[cursor++] = window[offset++];
                    count--;
                    x++;
                }
            } else {
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
            tmp_ptr -= 4096;
            memcpyl(buf->ptr, tmp_ptr, w * 8);
            buf->ptr += w * 8;
        }
    }
}



#if 0

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//uint32_t start = *(volatile uint32_t *)CS1(0x1014);	
	static const short kBits = 12;
	static const short kMask = (1 << kBits) - 1;

	unsigned short cursor = 0;
	short bits = 1;
	uint8_t count = 0;
	unsigned short offset = 0;
	static uint8_t window[(3 << kBits)] __attribute__ ((aligned (4)));
	uint8_t *tmp_ptr = (uint8_t *)window+4096;
//slTVOff();
	for (unsigned short y = 0; y < h; ++y) {
		unsigned short x = 0;

		while(x++ < w)
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
					const uint8_t color = *src++;
					window[cursor++] = color;
					*tmp_ptr++=color;
					cursor &= kMask;
					continue;
				}
				offset = READ_BE_UINT16(src); src += 2;
				count = 3 + (offset >> 12);
				offset &= kMask;
				offset = (cursor - offset - 1) & kMask;
			}

			const short tt = kMask-count;

			if(cursor<=tt && offset<=tt)
			{
#if 1				
				do
				{
					if(count>16 && x+16<w)
					{
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];

						count-=16;
						x+=16;
					}					
					
					if(count>8 && x+8<w)
					{
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						count-=8;
						x+=8;
					}
					
					if(count>4 && x+4<w)
					{
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						count-=4;
						x+=4;
					}
					if(count>2 && x+2<w)
					{
						*tmp_ptr++ = window[cursor++] = window[offset++];
						*tmp_ptr++ = window[cursor++] = window[offset++];
						count-=2;
						x+=2;
					}
					
					else
					{			
						*tmp_ptr++ = window[cursor++] = window[offset++];
						count--;
						x++;
					}
					x--;
				}while(count >0 && x++<w);
#else
				unsigned short rr2=(x+count<w)?count:1+((x+count)&512)-x;
				unsigned short count2 = (x+count<w)?0:count-rr2;
				
				memcpy(tmp_ptr,&window[offset],rr2);
				tmp_ptr+=rr2;
				cursor+=rr2;
				offset+=rr2;
				x+=rr2-1;
				count=count2;				
#endif	
			}
			else
			{
//				nb_2++;
				do
				{
					uint8_t color = window[offset++];
					window[cursor++] = color;
					*tmp_ptr++=color;
					cursor &= kMask;
					offset &= kMask;
					count--;					
				}while(count >0 && x++<w);
			}
		}
		if((y & 7) == 7)
		{
			tmp_ptr-=4096;
			memcpyl(buf->ptr,tmp_ptr,w*8);
//			slTransferEntry((void *)tmp_ptr,(void *)buf->ptr,w*32);
			buf->ptr+=w*8;

		}
//		DMA_ScuMemCopy((uint8*)&buf->ptr[y*w],(uint8*)tmp,  w);
//slTransferEntry((void *)tmp,(void *)&buf->ptr[y*w],w);
	}
//	slTVOn();
//emu_printf("nb0 no overlap %d overlap = %d\n", nb_0,nb_1);
//emu_printf("nb0 %d loop1 = %d loop2 %d\n", nb_0,nb_1,nb_2);
//emu_printf("nb1c %d nb1w = %d nb1l %d\n", nb_1,nb_1w,nb_1l);
//emu_printf("nb0 = %d nb1 %d nb2 = %d nb3 %d nb4 %d\n",nb_0, nb_1,nb_2,nb_3,nb_4);

//    uint32_t end = *(volatile uint32_t *)CS1(0x1014);

//    emu_printf("time = %d start %d %d\n", end-start,start,end);
}
#endif
/*
void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
	
	static const short kBits = 12;
	static const short kMask = (1 << kBits) - 1;

	unsigned short cursor = 0;
	short bits = 1;
	uint8_t count = 0;
	unsigned short offset = 0;
	uint8_t window[(1 << kBits)] __attribute__ ((aligned (4)));
	uint8_t tmp[4096] __attribute__ ((aligned (4)));
	uint8_t *tmp_ptr = (uint8_t *)tmp;
slTVOff();

	for (unsigned short y = 0; y < h; ++y) {

		for (unsigned short x = 0; x < w; ++x) {
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
					const uint8_t color = *src++;
					window[cursor] = color;
					++cursor;
					cursor &= kMask;
					tmp_ptr[x]=color;
					continue;
				}
				offset = READ_BE_UINT16(src); src += 2;
				count = 3 + (offset >> 12);
				offset &= kMask;
				offset = (cursor - offset - 1) & kMask;
			}
			const uint8_t color = window[offset++];
			offset &= kMask;
			window[cursor++] = color;
			cursor &= kMask;
			tmp_ptr[x]=color;
			
			--count;
		}
		tmp_ptr+=w;
		if((y & 7) == 7)
		{
//			DMA_ScuMemCopy(buf->ptr,tmp,w*8);
			memcpyl(buf->ptr,tmp,w*8);
//			memcpy(buf->ptr,tmp,w*8);
//			slTransferEntry((void *)tmp,(void *)buf->ptr,w*8);
			buf->ptr+=w*8;
			tmp_ptr=(uint8_t *)tmp;
		}
//		DMA_ScuMemCopy((uint8*)&buf->ptr[y*w],(uint8*)tmp,  w);
//slTransferEntry((void *)tmp,(void *)&buf->ptr[y*w],w);
//		SCU_DMAWait();
	}
	slTVOn();
}*/

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