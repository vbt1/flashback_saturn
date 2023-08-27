
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "unpack.h"
#include "util.h"

struct UnpackCtx {
	int size, datasize;
	uint32_t crc;
	uint32_t chk;
	uint32_t bits;
	uint8_t *dst;
	const uint8_t *src;
};

static int nextBit(UnpackCtx *uc) {
	int bit = (uc->bits & 1);
	uc->bits >>= 1;
	if (uc->bits == 0) { // getnextlwd
		const uint32_t bits = READ_BE_UINT32(uc->src); uc->src -= 4;
		uc->crc ^= bits;
		bit = (bits & 1);
		uc->bits = (1 << 31) | (bits >> 1);
	}
	return bit;
}

template<int count>
static uint32_t getBits(UnpackCtx *uc) { // rdd1bits
	uint32_t bits = 0;
	for (int i = 0; i < count; ++i) {
		bits = (bits << 1) | nextBit(uc);
	}
	return bits;
}

static void copyLiteral(UnpackCtx *uc, int len) { // getd3chr
	uc->size -= len;
	if (uc->size < 0) {
		len += uc->size;
		uc->size = 0;
	}
	for (int i = 0; i < len; ++i, --uc->dst) {
		*(uc->dst) = (uint8_t)getBits<8>(uc);
	}
}

static void copyReference(UnpackCtx *uc, int len, int offset) { // copyd3bytes
	uc->size -= len;
	if (uc->size < 0) {
		len += uc->size;
		uc->size = 0;
	}
	for (int i = 0; i < len; ++i, --uc->dst) {
		*(uc->dst) = *(uc->dst + offset);
	}
}



static int rcr(UnpackCtx *uc, int CF) {
	int rCF = (uc->chk & 1);
	uc->chk >>= 1;
	if (CF) {
		uc->chk |= 0x80000000;
	}
	return rCF;
}

static int next_chunk(UnpackCtx *uc) {
	int CF = rcr(uc, 0);
	if (uc->chk == 0) {
		uc->chk = READ_BE_UINT32(uc->src); uc->src -= 4;
		uc->crc ^= uc->chk;
		CF = rcr(uc, 1);
	}
	return CF;
}

static uint16 get_code(UnpackCtx *uc, uint8 num_chunks) {
	uint16 c = 0;
	while (num_chunks--) {
		c <<= 1;
		if (next_chunk(uc)) {
			c |= 1;
		}
	}
	return c;
}

static void dec_unk1(UnpackCtx *uc, uint8 num_chunks, uint8 add_count) {
	uint16 count = get_code(uc, num_chunks) + add_count + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = (uint8)get_code(uc, 8);
		--uc->dst;
	}
}

static void dec_unk2(UnpackCtx *uc, uint8 num_chunks) {
	uint16 i = get_code(uc, num_chunks);
	uint16 count = uc->size + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = *(uc->dst + i);
		--uc->dst;
	}
}

bool delphine_unpack(uint8 *dst, const uint8 *src, int len) {
	UnpackCtx uc;
	uc.src = src + len - 4;
	uc.datasize = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.dst = dst + uc.datasize - 1;
	uc.size = 0;
	uc.crc = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.chk = READ_BE_UINT32(uc.src); uc.src -= 4;
	debug(DBG_UNPACK, "delphine_unpack() crc=0x%X datasize=0x%X", uc.crc, uc.datasize);
	uc.crc ^= uc.chk;
	do {
		if (!next_chunk(&uc)) {
			uc.size = 1;
			if (!next_chunk(&uc)) {
				dec_unk1(&uc, 3, 0);
			} else {
				dec_unk2(&uc, 8);
			}
		} else {
			uint16 c = get_code(&uc, 2);
			if (c == 3) {
				dec_unk1(&uc, 8, 8);
			} else if (c < 2) {
				uc.size = c + 2;
				dec_unk2(&uc, c + 9);
			} else {
				uc.size = get_code(&uc, 8);
				dec_unk2(&uc, 12);
			}
		}
	} while (uc.datasize > 0);
	return uc.crc == 0;
}

bool bytekiller_unpack(uint8_t *dst, int dstSize, const uint8_t *src, int srcSize) {
	UnpackCtx uc;
	uc.src = src + srcSize - 4;
	uc.size = READ_BE_UINT32(uc.src); uc.src -= 4;
	if (uc.size > dstSize) {
		warning("Unexpected unpack size %d, buffer size %d", uc.size, dstSize);
		return false;
	}
	uc.dst = dst + uc.size - 1;
	uc.crc = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.bits = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.crc ^= uc.bits;
	do {
		if (!nextBit(&uc)) {
			if (!nextBit(&uc)) {
				copyLiteral(&uc, getBits<3>(&uc) + 1);
			} else {
				copyReference(&uc, 2, getBits<8>(&uc));
			}
		} else {
			const int code = getBits<2>(&uc);
			switch (code) {
			case 3:
				copyLiteral(&uc, getBits<8>(&uc) + 9);
				break;
			case 2: {
					const int len = getBits<8>(&uc) + 1;
					copyReference(&uc, len, getBits<12>(&uc));
				}
				break;
			case 1:
				copyReference(&uc, 4, getBits<10>(&uc));
				break;
			case 0:
				copyReference(&uc, 3, getBits<9>(&uc));
				break;
			}
		}
	} while (uc.size > 0);
	assert(uc.size == 0);
	return uc.crc == 0;
}
