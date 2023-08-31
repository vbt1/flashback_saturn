/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2007 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
extern "C" {
#include <sega_mem.h>
#include "sat_mem_checker.h"
#include "saturn_print.h"
}
#include "file.h"
#include "decode_mac.h"
#include "resource.h"
#include "systemstub.h"
#include "video.h"


Video::Video(Resource *res, SystemStub *stub)
	: _res(res), _stub(stub) {
	_layerScale = (_res->_type == kResourceTypeMac) ? 2 : 1; // Macintosh version is 512x448
	_w = GAMESCREEN_W * _layerScale;
	_h = GAMESCREEN_H * _layerScale;		
	//_frontLayer = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_frontLayer, 0, _w * _h);
	//_backLayer = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_backLayer, 0, _w * _h);
	//_tempLayer = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_tempLayer, 0, _w * _h);
	//_tempLayer2 = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
//	memset(_tempLayer2, 0, _w * _h);
	//_screenBlocks = (uint8 *)sat_malloc((GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
	memset(_screenBlocks, 0, (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H));
	_fullRefresh = true;
	_shakeOffset = 0;
	_charFrontColor = 0;
	_charTransparentColor = 0;
	_charShadowColor = 0;
	//_drawChar = 0;
	switch (_res->_type) {
	case kResourceTypeDOS:
		_drawChar = &Video::PC_drawStringChar;
		break;
	case kResourceTypeMac:
		_drawChar = &Video::MAC_drawStringChar;
		
	emu_printf("VBT VBT MAC_drawStringChar init \n");		
		break;
	}	
}

Video::~Video() {
	//sat_free(_frontLayer);
	//sat_free(_backLayer);
	//sat_free(_tempLayer);
	//sat_free(_tempLayer2);
	//sat_free(_screenBlocks);
}
/*
void Video::markBlockAsDirty(int16 x, int16 y, uint16 w, uint16 h) {
	debug(DBG_VIDEO, "Video::markBlockAsDirty1(%d, %d, %d, %d)", x, y, w, h);
	assert(x >= 0 && x + w <= GAMESCREEN_W && y >= 0 && y + h <= GAMESCREEN_H);
	int bx1 = x / SCREENBLOCK_W;
	int by1 = y / SCREENBLOCK_H;
	int bx2 = (x + w - 1) / SCREENBLOCK_W;
	int by2 = (y + h - 1) / SCREENBLOCK_H;
	assert(bx2 < GAMESCREEN_W / SCREENBLOCK_W && by2 < GAMESCREEN_H / SCREENBLOCK_H);
	for (; by1 <= by2; ++by1) {
		for (int i = bx1; i <= bx2; ++i) {
			_screenBlocks[by1 * (GAMESCREEN_W / SCREENBLOCK_W) + i] = 2;
		}
	}
}*/

void Video::markBlockAsDirty(int16_t x, int16_t y, uint16_t w, uint16_t h, int scale) {
	debug(DBG_VIDEO, "Video::markBlockAsDirty2(%d, %d, %d, %d)", x, y, w, h);
	int bx1 = scale * x / SCREENBLOCK_W;
	int by1 = scale * y / SCREENBLOCK_H;
	int bx2 = scale * (x + w - 1) / SCREENBLOCK_W;
	int by2 = scale * (y + h - 1) / SCREENBLOCK_H;
	if (bx1 < 0) {
		bx1 = 0;
	}
	if (bx2 > (_w / SCREENBLOCK_W) - 1) {
		bx2 = (_w / SCREENBLOCK_W) - 1;
	}
	if (by1 < 0) {
		by1 = 0;
	}
	if (by2 > (_h / SCREENBLOCK_H) - 1) {
		by2 = (_h / SCREENBLOCK_H) - 1;
	}
	for (; by1 <= by2; ++by1) {
		for (int i = bx1; i <= bx2; ++i) {
			_screenBlocks[by1 * (_w / SCREENBLOCK_W) + i] = 2;
		}
	}
}

void Video::updateScreen() {
	debug(DBG_VIDEO, "Video::updateScreen()");
//	_fullRefresh = true;
//_shakeOffset=0;
	if (_fullRefresh) {
		_stub->copyRect(0, 0, Video::GAMESCREEN_W*2, Video::GAMESCREEN_H*2, _frontLayer, 512); // vbt 512 au lieu de 256
		_stub->updateScreen(_shakeOffset);
		_fullRefresh = false;
	} else {
		int i, j;
		int count = 0;
		uint8 *p = _screenBlocks;
		for (j = 0; j < GAMESCREEN_H*2 / SCREENBLOCK_H; ++j) {
			uint16 nh = 0;
			for (i = 0; i < GAMESCREEN_W*2 / SCREENBLOCK_W; ++i) {
				if (p[i] != 0) {
					--p[i];
					++nh;
				} else if (nh != 0) {
					int16 x = (i - nh) * SCREENBLOCK_W;
					_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, 512);
					nh = 0;
					++count;
				}
			}
			if (nh != 0) {
				int16 x = (i - nh) * SCREENBLOCK_W;
				_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, 512);
				++count;
			}
			p += GAMESCREEN_W*2 / SCREENBLOCK_W;
		}
		if (count != 0) {
			_stub->updateScreen(_shakeOffset);
		}
	}
	if (_shakeOffset != 0) {
		_shakeOffset = 0;
		_fullRefresh = true;
	}
}

void Video::fullRefresh() {
	debug(DBG_VIDEO, "Video::fullRefresh()");
	_fullRefresh = true;
	memset(_screenBlocks, 0, (GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
}

void Video::fadeOut() {
	debug(DBG_VIDEO, "Video::fadeOut()");
	for (int step = 16; step >= 0; --step) {
		for (int c = 0; c < 256; ++c) {
			Color col;
			_stub->getPaletteEntry(c, &col);
			col.r = col.r * step >> 4;
			col.g = col.g * step >> 4;
			col.b = col.b * step >> 4;
			_stub->setPaletteEntry(c, &col);
		}
		fullRefresh();
		updateScreen();
		_stub->sleep(50);
	}
}

void Video::setPaletteSlotBE(int palSlot, int palNum) {
	debug(DBG_VIDEO, "Video::setPaletteSlotBE()");
	const uint8 *p = _res->_pal + palNum * 0x20;
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_BE_UINT16(p); p += 2;
		uint8 t = (color == 0) ? 0 : 3;
		Color c;

		c.r = ((color & 0x00F) << 2) | t;
		c.g = ((color & 0x0F0) >> 2) | t;
		c.b = ((color & 0xF00) >> 6) | t;
	
		_stub->setPaletteEntry(palSlot * 0x10 + i, &c);
	}
}

void Video::setPaletteSlotLE(int palSlot, const uint8 *palData) {
	debug(DBG_VIDEO, "Video::setPaletteSlotLE()");
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_LE_UINT16(palData); palData += 2;
		Color c;
		c.b = (color & 0x00F) << 2;
		c.g = (color & 0x0F0) >> 2;
		c.r = (color & 0xF00) >> 6;
		_stub->setPaletteEntry(palSlot * 0x10 + i, &c);
	}
}

void Video::setTextPalette() {
	debug(DBG_VIDEO, "Video::setTextPalette()");
	const uint8 *p = _textPal;
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_LE_UINT16(p); p += 2;
		Color c;
		c.b = (color & 0x00F) << 2;
		c.g = (color & 0x0F0) >> 2;
		c.r = (color & 0xF00) >> 6;
		_stub->setPaletteEntry(0xE0 + i, &c);
	}
}

void Video::setPalette0xF() {
	debug(DBG_VIDEO, "Video::setPalette0xF()");
	const uint8 *p = _palSlot0xF;
	for (int i = 0; i < 16; ++i) {
		Color c;
		c.r = *p++ >> 2;
		c.g = *p++ >> 2;
		c.b = *p++ >> 2;
		_stub->setPaletteEntry(0xF0 + i, &c);
	}
}

void Video::copyLevelMap(uint16 room) {
	debug(DBG_VIDEO, "Video::copyLevelMap(%d)", room);
	assert(room < 0x40);
#ifdef _RAMCART_
	int32 off = READ_LE_UINT32(_res->_map + room * 6);
#else
	File f;
	f.open(_res->_mapFilename, "./", "wb");
	f.seek(room * 6);

	int32 off = f.readUint32LE();
#endif
	if (off == 0) {
		error("Invalid room %d", room);
	}
	bool packed = true;
	if (off < 0) {
		off = -off;
		packed = false;
	}
#ifdef _RAMCART_
	const uint8 *p = _res->_map + off;
	_mapPalSlot1 = *p++;
	_mapPalSlot2 = *p++;
	_mapPalSlot3 = *p++;
	_mapPalSlot4 = *p++;
#else
	f.seek(off);
	_mapPalSlot1 = f.readByte();
	_mapPalSlot2 = f.readByte();
	_mapPalSlot3 = f.readByte();
	_mapPalSlot4 = f.readByte();
#endif
	if (packed) {
		uint8 *vid = _frontLayer;
		for (int i = 0; i < 4; ++i) {
#ifdef _RAMCART_
			uint16 sz = READ_LE_UINT16(p); p += 2;
			decodeLevelMap(sz, p, _res->_memBuf); p += sz;
#else
			uint16 sz = f.readUint16LE();
			uint8 *tmpbuf = (uint8*)sat_malloc(sz);
			f.read(tmpbuf, sz);
			decodeLevelMap(sz, tmpbuf, _res->_memBuf);
			sat_free(tmpbuf);
#endif
			memcpy(vid, _res->_memBuf, 256 * 56);
			vid += 256 * 56;
		}
	} else {
		for (int i = 0; i < 4; ++i) {
			for (int y = 0; y < 224; ++y) {
				for (int x = 0; x < 64; ++x) {
#ifdef _RAMCART_
					_frontLayer[i + x * 4 + 256 * y] = p[256 * 56 * i + x + 64 * y];
#else
					f.seek(256 * 56 * i + x + 64 * y);
					uint8 dat = f.readByte();
					_frontLayer[i + x * 4 + 256 * y] = dat;
#endif

				}
			}
		}
	}
	memcpy(_backLayer, _frontLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);

#ifdef _RAMCART_
	f.close();
#endif
}

void Video::decodeLevelMap(uint16 sz, const uint8 *src, uint8 *dst) {
	debug(DBG_VIDEO, "Video::decodeLevelMap() sz = 0x%X", sz);
	const uint8 *end = src + sz;
	while (src < end) {
		int16 code = (int8)*src++;
		if (code < 0) {
			int len = 1 - code;
			memset(dst, *src++, len);
			dst += len;
		} else {
			++code;
			memcpy(dst, src, code);
			src += code;
			dst += code;
		}
	}
}

void Video::setLevelPalettes() {
	debug(DBG_VIDEO, "Video::setLevelPalettes()");
	if (_unkPalSlot2 == 0) {
		_unkPalSlot2 = _mapPalSlot3;
	}
	if (_unkPalSlot1 == 0) {
		_unkPalSlot1 = _mapPalSlot3;
	}
	setPaletteSlotBE(0x0, _mapPalSlot1);
	setPaletteSlotBE(0x1, _mapPalSlot2);
	setPaletteSlotBE(0x2, _mapPalSlot3);
	setPaletteSlotBE(0x3, _mapPalSlot4);
	if (_unkPalSlot1 == _mapPalSlot3) {
		setPaletteSlotLE(4, _conradPal1);
	} else {
		setPaletteSlotLE(4, _conradPal2);
	}
	// slot 5 is monster palette
	setPaletteSlotBE(0x8, _mapPalSlot1);
	setPaletteSlotBE(0x9, _mapPalSlot2);
	setPaletteSlotBE(0xA, _unkPalSlot2);
	setPaletteSlotBE(0xB, _mapPalSlot4);
	// slots 0xC and 0xD are cutscene palettes
	setTextPalette();
}

void Video::drawSpriteSub1(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub1(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i] != 0) {
				dst[i] = src[i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub2(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub2(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i] != 0) {
				dst[i] = src[-i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub3(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub3(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub4(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub4(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[-i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub5(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub5(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i * pitch] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[i * pitch] | colMask;
			}
		}
		++src;
		dst += 256;
	}
}

void Video::drawSpriteSub6(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub6(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i * pitch] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[-i * pitch] | colMask;
			}
		}
		++src;
		dst += 256;
	}
}

void Video::drawChar(uint8 c, int16 y, int16 x) {
//	emu_printf("Video::drawChar(0x%X, %d, %d)\n", c, y, x);
	y *= 8;
	x *= 8;
	const uint8 *src = _res->_fnt + (c - 32) * 32;
	uint8 *dst = _frontLayer + x + 256 * y;
	for (int h = 0; h < 8; ++h) {
		for (int i = 0; i < 4; ++i) {
			uint8 c1 = (*src & 0xF0) >> 4;
			uint8 c2 = (*src & 0x0F) >> 0;
			++src;

			if (c1 != 0) {
				if (c1 != 2) {
					*dst = _charFrontColor;
				} else {
					*dst = _charShadowColor;
				}
			} else if (_charTransparentColor != 0xFF) {
				*dst = _charTransparentColor;
			}
			++dst;

			if (c2 != 0) {
				if (c2 != 2) {
					*dst = _charFrontColor;
				} else {
					*dst = _charShadowColor;
				}
			} else if (_charTransparentColor != 0xFF) {
				*dst = _charTransparentColor;
			}
			++dst;
		}
		dst += 256 - 8;
	}
}

void Video::PC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
	dst += y * pitch + x;
	assert(chr >= 32);
	src += (chr - 32) * 8 * 4;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 4; ++x) {
			const uint8_t c1 = src[x] >> 4;
			if (c1 != 0) {
				*dst = (c1 == 15) ? color : (0xE0 + c1);
			}
			++dst;
			const uint8_t c2 = src[x] & 15;
			if (c2 != 0) {
				*dst = (c2 == 15) ? color : (0xE0 + c2);
			}
			++dst;
		}
		src += 4;
		dst += pitch - CHAR_W;
	}
}

static uint8_t _MAC_fontFrontColor;
static uint8_t _MAC_fontShadowColor;

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
	//emu_printf("Video::MAC_drawStringChar\n");	
	DecodeBuffer buf;
	
//emu_printf("a\n");	
	memset(&buf, 0, sizeof(buf));
//		int *val = (int *)dst;	
//emu_printf("b %08x\n",(int)val);	
	buf.ptr = dst;
//emu_printf("c\n");	
	buf.w = _w;
	buf.pitch = _w;
	buf.h = _h;
	buf.x = x * _layerScale;
	buf.y = y * _layerScale;
	
//	emu_printf("Video::drawString('w %d h %d x %d y %d p %d scale%d chr %c)\n", _w,_h,x,y,buf.pitch,_layerScale,chr);	
//emu_printf("d\n");		
	buf.setPixel = Video::MAC_setPixelFont;
	_MAC_fontFrontColor = color;
	_MAC_fontShadowColor = _charShadowColor;
//emu_printf("e\n");		
	assert(chr >= 32);
//emu_printf("f\n");		
	_res->MAC_decodeImageData(_res->_fnt, chr - 32, &buf);
//emu_printf("g\n");		
}

const char *Video::drawString(const char *str, int16_t x, int16_t y, uint8_t col) {
//	emu_printf("Video::drawString('%s', %d, %d, 0x%X)\n", str, x, y, col);
	const uint8_t *fnt = _res->_fnt;
	int len = 0;
	while (1) {
		const uint8_t c = *str++;
		if (c == 0 || c == 0xB || c == 0xA) {
			break;
		}
//	emu_printf("avant _drawChar dc null? %p fladdr %x w %d x+l %d y %d fntaddr %x col %d c %02x\n",_drawChar,_frontLayer, _w, x + len * CHAR_W, y, fnt, col, c);

		(this->*_drawChar)(_frontLayer, _w, x + len * CHAR_W, y, fnt, col, c);
//emu_printf("apres _drawChar\n");		
		++len;
	}
	//markBlockAsDirty(x, y, len * CHAR_W, CHAR_H, _layerScale);
	return str - 1;
}


void Video::MAC_decodeMap(int level, int room) {
	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.ptr = _frontLayer;
	buf.w = buf.pitch = _w;
	buf.h = _h;
	buf.setPixel = Video::MAC_setPixel;
	_res->MAC_loadLevelRoom(level, room, &buf);
	memcpy(_backLayer, _frontLayer, _layerSize);
	Color roomPalette[256];
	_res->MAC_setupRoomClut(level, room, roomPalette);
	for (int j = 0; j < 16; ++j) {
		if (j == 5 || j == 7 || j == 14 || j == 15) {
			continue;
		}
		for (int i = 0; i < 16; ++i) {
			const int color = j * 16 + i;
			_stub->setPaletteEntry(color, &roomPalette[color]);
		}
	}
}

void Video::MAC_setPixel(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	buf->ptr[offset] = color;
}

void Video::MAC_setPixelMask(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	if ((buf->ptr[offset] & 0x80) == 0) {
		buf->ptr[offset] = color;
	}
}

void Video::MAC_setPixelFont(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	switch (color) {
	case 0xC0:
		buf->ptr[offset] = _MAC_fontShadowColor;
		break;
	case 0xC1:
		buf->ptr[offset] = _MAC_fontFrontColor;
		break;
	}
}

void Video::fillRect(int x, int y, int w, int h, uint8_t color) {
	uint8_t *p = _frontLayer + y * _layerScale * _w + x * _layerScale;
	for (int j = 0; j < h * _layerScale; ++j) {
		memset(p, color, w * _layerScale);
		p += _w;
	}
}

static void fixOffsetDecodeBuffer(DecodeBuffer *buf, const uint8_t *dataPtr) {
        if (buf->xflip) {
		buf->x += (int16_t)READ_BE_UINT16(dataPtr + 4) - READ_BE_UINT16(dataPtr) - 1;
        } else {
		buf->x -= (int16_t)READ_BE_UINT16(dataPtr + 4);
        }
        buf->y -= (int16_t)READ_BE_UINT16(dataPtr + 6);
}

void Video::MAC_drawSprite(int x, int y, const uint8_t *data, int frame, bool xflip, bool eraseBackground) {
	const uint8_t *dataPtr = _res->MAC_getImageData(data, frame);
	if (dataPtr) {
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.xflip = xflip;
		buf.ptr = _frontLayer;
		buf.w = buf.pitch = _w;
		buf.h = _h;
		buf.x = x * _layerScale;
		buf.y = y * _layerScale;
		buf.setPixel = eraseBackground ? MAC_setPixel : MAC_setPixelMask;
		fixOffsetDecodeBuffer(&buf, dataPtr);
		_res->MAC_decodeImageData(data, frame, &buf);
		markBlockAsDirty(buf.x, buf.y, READ_BE_UINT16(dataPtr), READ_BE_UINT16(dataPtr + 2), 1);
	}
}
