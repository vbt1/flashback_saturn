
#ifndef DECODE_MAC_H__
#define DECODE_MAC_H__

#include <stdint.h>
#include "file.h"
#include "resource_mac.h"
uint8_t* decodeLzss(File& f, const uint8_t type, const uint16_t id, uint32_t& decodedSize, const ResourceMacEntry *entry);
//uint8_t* decodeLzss(File& f, const ResourceMacEntry *entry);
uint8_t* decodeLzssCache(File& f, const uint8_t type, const uint16_t id, uint32_t& decodedSize);

struct DecodeBuffer {
	uint8_t *ptr __attribute__((aligned(2)));
	uint8_t type;
	int dst_w, dst_h;
	int dst_x, dst_y;
	bool xflip;
//	void (*setPixel)(DecodeBuffer *buf, int x, int y, uint8_t color);
//	void (*setPixel)(DecodeBuffer *buf, uint16_t x, uint16_t y, uint8_t color);
};

void decodeC103(const uint8_t *a3, int w, int h, DecodeBuffer *buf, unsigned char mask);
void decodeC211(const uint8_t *a3, int w, int h, DecodeBuffer *buf);
void remap_colors();

#endif
