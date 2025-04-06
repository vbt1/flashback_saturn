
#ifndef DECODE_MAC_H__
#define DECODE_MAC_H__

#include <stdint.h>
#include "file.h"

uint8_t *decodeLzss(File &f, const char *name, uint32_t &decodedSize);

struct DecodeBuffer {
	uint8_t *ptr;
	int dst_w, dst_h;
	int dst_x, dst_y;
	int pitch;
	bool xflip;
	uint8_t type;
	int orig_w, orig_h;
	void (*setPixel)(DecodeBuffer *buf, int x, int y, uint8_t color);
//	void *dataPtr;
	int clip_x, clip_y;
	int clip_w, clip_h;
	uint8_t *clip_buf;
};

void decodeC103(const uint8_t *a3, int w, int h, DecodeBuffer *buf, unsigned char mask);
void decodeC211(const uint8_t *a3, int w, int h, DecodeBuffer *buf);

#endif
