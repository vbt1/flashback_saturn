//#define WITH_MEM_MALLOC 1
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

emu_printf("lzss %s %d\n", name, decodedSize);
	decodedSize = f.readUint32BE();
	
	uint8_t *dst;
#ifdef WITH_MEM_MALLOC
	if(strstr(name,"conditions") != NULL 
	 || strstr(name," map") != NULL
	 )
#else
	if(strstr(name,"conditions") != NULL 
	 || strstr(name," map") != NULL
	 )
#endif
	{
#ifdef WITH_MEM_MALLOC
		dst = (uint8_t *)sat_malloc(decodedSize);
#else
//		dst = (uint8_t *)sat_malloc(decodedSize);
		dst = (uint8_t *)current_lwram;
		current_lwram += SAT_ALIGN(decodedSize);
#endif
	}
	else
	{
		if(strstr(name,"Room") != NULL)
		{
			if(hwram_screen==NULL)
			{
				hwram_screen=hwram_ptr;
				hwram_ptr+=45000;
			}
			//emu_printf("hwram1 %d %s\n", decodedSize, name);
			dst = (uint8_t *)hwram_screen;
		}
		else
		{
#ifdef WITH_MEM_MALLOC
			if ((int)hwram_ptr+decodedSize<=end1 && strncmp("Icons", name, 5) != 0 )
			{
				//emu_printf("hwram %d %s\n", decodedSize, name);
				dst = (uint8_t *)hwram_ptr;
				hwram_ptr += SAT_ALIGN(decodedSize);
			}
			else
			{
				//emu_printf("lwram %d %s end %d\n", decodedSize, name,end1);
				dst = (uint8_t *)sat_malloc(decodedSize);
			}
#else
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
#endif
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
//	emu_printf("dst %p\n",dst);
	
	if(count != decodedSize)  // vbt ne pas toucher
	{
//		emu_printf("count != decodedSize  %d %d\n", count, decodedSize);
		return dst;		
	}
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

void decodeC103(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
	
	static const short kBits = 12;
	static const short kMask = (1 << kBits) - 1;
	unsigned short cursor = 0;
	short bits = 1;
	uint8_t count = 0;
	unsigned short offset = 0;
	uint8_t window[(1 << kBits)] __attribute__ ((aligned (4)));
	uint8_t tmp[4096] __attribute__ ((aligned (4)));
	uint8_t *tmp_ptr = NULL;

slTVOff();

	for (unsigned short y = 0; y < h; ++y) {
		tmp_ptr=(uint8_t *)tmp+(w*(y&7));

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
//					setPixel(x, y, w, h, color, buf);
					tmp_ptr[x]=color;
					continue;
				}
				offset = READ_BE_UINT16(src); src += 2;
//offset = ((uint16_t*)src)[0]; src += 2;				
				count = 3 + (offset >> 12);
				offset &= kMask;
				offset = (cursor - offset - 1) & kMask;
			}
			const uint8_t color = window[offset++];
			offset &= kMask;
			window[cursor++] = color;
			cursor &= kMask;
//			setPixel(x, y, w, h, color, buf);
			tmp_ptr[x]=color;
			--count;
		}
		if((y & 7) == 7)
		{
//			DMA_ScuMemCopy(buf->ptr,tmp,w*8);
			memcpyl(buf->ptr,tmp,w*8);
//			memcpy(buf->ptr,tmp,w*8);
//			slTransferEntry((void *)tmp,(void *)buf->ptr,w*8);
			buf->ptr+=w*8;
		}

//		DMA_ScuMemCopy((uint8*)&buf->ptr[y*w],(uint8*)tmp,  w);
//slTransferEntry((void *)tmp,(void *)&buf->ptr[y*w],w);
//		SCU_DMAWait();
	}
	slTVOn();
}

void decodeC211(const uint8_t *src, int w, int h, DecodeBuffer *buf) {
//emu_printf("decodeC211 w %d h %d %p %p\n",w,h,buf->ptr,buf->ptrsp);
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
}