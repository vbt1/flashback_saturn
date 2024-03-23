
#ifndef DECODE_MAC_H__
#define DECODE_MAC_H__

#include <stdint.h>
#include "file.h"

uint8_t *decodeLzss(File &f, const char *name, const uint8_t *_scratchBuffer, uint32_t &decodedSize);

struct DecodeBuffer {
	uint8_t *ptr;
	uint8_t *ptrsp;
	int w, h, pitch;
	int w2, h2;
	int x, y;
	bool xflip;

	void (*setPixel)(DecodeBuffer *buf, int x, int y, uint8_t color);
	void *dataPtr;
};

void decodeC103(const uint8_t *a3, int w, int h, DecodeBuffer *buf);
void decodeC211(const uint8_t *a3, int w, int h, DecodeBuffer *buf);

#endif
