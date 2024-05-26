#define PRELOAD_MONSTERS 1
//#define COLOR_4BPP 1
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
//#define SLAVE_SOUND 1 
extern "C"
{
#include <sl_def.h>	
//#include "sega_mem.h"
#include "sat_mem_checker.h"
#include "saturn_print.h"
#include <string.h>
extern TEXTURE tex_spr[4];
extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
extern Uint8 *current_lwram;
extern Uint8 *hwram_screen;
void	*malloc(size_t);
}
#include "file.h"
#include "decode_mac.h"
#include "resource.h"
#include "systemstub.h"
#include "video.h"
/*
#undef VDP2_VRAM_A0
#define VDP2_VRAM_A0 NULL 
#undef VDP2_VRAM_B0
#define VDP2_VRAM_B0 NULL 
*/
#define LOW_WORK_RAM 0x00200000

Video::Video(Resource *res, SystemStub *stub)
	: _res(res), _stub(stub) {
		


	_layerScale = (_res->_type == kResourceTypeMac) ? 2 : 1; // Macintosh version is 512x448
	_w = GAMESCREEN_W * _layerScale;
	_h = GAMESCREEN_H * _layerScale;
//	_layerSize = _w * _h;
//emu_printf("_frontLayer = (uint8 *)sat_malloc(%d) \n",_w * _h);
	_frontLayer = (uint8 *)sat_malloc(_w * _h);

	memset(_frontLayer, 0, _w * _h);

	_backLayer = (uint8_t *)VDP2_VRAM_B0;
//	_backLayer = (uint8_t *)LOW_WORK_RAM;

//	_txt1Layer = (uint8_t *)(SpriteVRAM + TEXT1_RAM_VDP2);
//	_txt2Layer = (uint8_t *)(SpriteVRAM + TEXT2_RAM_VDP2);	
	memset(_backLayer, 0, _w * _h); // vbt à remettre
	
	//_tempLayer = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
//	memset(_tempLayer, 0, GAMESCREEN_W * GAMESCREEN_H);
//	_tempLayer2 = (uint8 *)sat_malloc(GAMESCREEN_W * GAMESCREEN_H);
//	memset(_tempLayer2, 0, GAMESCREEN_W * GAMESCREEN_H);
	//_screenBlocks = (uint8 *)sat_malloc((GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
//	_screenBlocks = (uint8 *)std_malloc((_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H)); //[(GAMESCREEN_W*2 / SCREENBLOCK_W) * (GAMESCREEN_H*2 / SCREENBLOCK_H)];
//		emu_printf("_screenBlocks %d %p\n", (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H), _screenBlocks);
//	memset(_screenBlocks, 0, (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H));
	
	_fullRefresh = true;
//	_shakeOffset = 0;
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
		break;
	}
}

Video::~Video() {
//	sat_free(_frontLayer);
//	sat_free(_backLayer);
//	sat_free(_tempLayer);
//	sat_free(_tempLayer2);
//	sat_free(_screenBlocks);
}
/*
void Video::markBlockAsDirty(int16_t x, int16_t y, uint16_t w, uint16_t h, int scale) {
//	emu_printf( "Video::markBlockAsDirty2(%d, %d, %d, %d)\n", x, y, w, h);
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
*/
void Video::updateScreen() {
	//	debug(DBG_VIDEO, "Video::updateScreen()");
	
//	_stub->updateScreen(0);
//	emu_printf("Video::fullRefresh %d\n",_fullRefresh);	
//		memset(_screenBlocks, 1, (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H));
//	_fullRefresh = false;
//_shakeOffset=0;
	if (_fullRefresh) {
		_stub->copyRect(0, 0, _w, _h, _frontLayer, _w);
		_stub->updateScreen(0);
		_fullRefresh = false;
	} 
/*	
	else {
		int i, j;
		int count = 0;
		uint8_t *p = _screenBlocks;
		for (j = 0; j < _h / SCREENBLOCK_H; ++j) {
			uint16_t nh = 0;
			for (i = 0; i < _w / SCREENBLOCK_W; ++i) {
				if (p[i] != 0) {
					--p[i];
					++nh;
				} else if (nh != 0) {
					int16_t x = (i - nh) * SCREENBLOCK_W;
					_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, _w);
					nh = 0;
					++count;
				}
			}
			if (nh != 0) {
				int16_t x = (i - nh) * SCREENBLOCK_W;
				_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, _w);
				++count;
			}
			p += _w / SCREENBLOCK_W;
		}
//		if (count != 0) {
//			_stub->updateScreen(0);  // vbt : évite une copie
//		}
	}
*/	
//	if (_shakeOffset != 0) 
	{
//		_shakeOffset = 0;
//		_fullRefresh = true;
	}
}

void Video::fullRefresh() {
//	emu_printf("Video::fullRefresh()\n");	
	_fullRefresh = true;
//	memset(_screenBlocks, 0, (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H));
}

void Video::fadeOut() {
//	emu_printf("Video::fadeOut()\n");	
	if (1) {
		fadeOutPalette();
	} else {
//		_stub->fadeScreen();
	}
}

void Video::fadeOutPalette() {
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
/*
void Video::setPaletteSlotBE(int palSlot, int palNum) {
	//	debug(DBG_VIDEO, "Video::setPaletteSlotBE()");
	const uint8_t *p = _res->_pal + palNum * 32;
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
*/
void Video::setPaletteSlotLE(int palSlot, const uint8_t *palData) {
	//	debug(DBG_VIDEO, "Video::setPaletteSlotLE()");
	for (int i = 0; i < 16; ++i) {
		const uint16_t color = READ_LE_UINT16(palData); palData += 2;
		Color c;
		c.b = (color & 0x00F) << 2;
		c.g = (color & 0x0F0) >> 2;
		c.r = (color & 0xF00) >> 6;
		_stub->setPaletteEntry(palSlot * 0x10 + i, &c);
	}
}

void Video::setTextPalette() {
	//	debug(DBG_VIDEO, "Video::setTextPalette()");
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
	//	debug(DBG_VIDEO, "Video::setPalette0xF()");
	const uint8_t *p = _palSlot0xF;
	for (int i = 0; i < 16; ++i) {
		Color c;
		c.r = *p++ >> 2;
		c.g = *p++ >> 2;
		c.b = *p++ >> 2;
		_stub->setPaletteEntry(0xF0 + i, &c);
	}
}
/*
void Video::copyLevelMap(uint16 room) {
	//	debug(DBG_VIDEO, "Video::copyLevelMap(%d)", room);
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
			decodeLevelMap(sz, tmpbuf, _res->_scratchBuffer);
			sat_free(tmpbuf);
#endif
			memcpy(vid, _res->_scratchBuffer, 256 * 56);
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
//	memcpy(_backLayer, _frontLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);

#ifdef _RAMCART_
	f.close();
#endif
}
*/
void Video::decodeLevelMap(uint16 sz, const uint8 *src, uint8 *dst) {
	//	debug(DBG_VIDEO, "Video::decodeLevelMap() sz = 0x%X", sz);
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
/*
void Video::setLevelPalettes() {
	//	debug(DBG_VIDEO, "Video::setLevelPalettes()");
	if (_unkPalSlot2 == 0) {
		_unkPalSlot2 = _mapPalSlot3;
	}
	if (_unkPalSlot1 == 0) {
		_unkPalSlot1 = _mapPalSlot3;
	}
	// background
	setPaletteSlotBE(0x0, _mapPalSlot1);
	// objects
	setPaletteSlotBE(0x1, _mapPalSlot2);
	setPaletteSlotBE(0x2, _mapPalSlot3);
	setPaletteSlotBE(0x3, _mapPalSlot4);
	// conrad
	if (_unkPalSlot1 == _mapPalSlot3) {
		setPaletteSlotLE(4, _conradPal1);
	} else {
		setPaletteSlotLE(4, _conradPal2);
	}
	// slot 5 is monster palette
	// foreground
	setPaletteSlotBE(0x8, _mapPalSlot1);
	setPaletteSlotBE(0x9, _mapPalSlot2);
	// inventory
	setPaletteSlotBE(0xA, _unkPalSlot2);
	setPaletteSlotBE(0xB, _mapPalSlot4);
	// slots 0xC and 0xD are cutscene palettes
	setTextPalette();
}
*/
void Video::drawSpriteSub1(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
//	emu_printf("Video::drawSpriteSub1(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::drawSpriteSub2(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask) {
//	emu_printf("Video::drawSpriteSub2(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::drawSpriteSub3(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask) {
//	emu_printf("Video::drawSpriteSub3(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::drawSpriteSub4(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask) {
//	emu_printf("Video::drawSpriteSub4(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::drawSpriteSub5(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask) {
//	emu_printf("Video::drawSpriteSub5(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::drawSpriteSub6(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask) {
//	emu_printf("Video::drawSpriteSub6(0x%X, 0x%X, 0x%X, 0x%X)\n", pitch, w, h, colMask);
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

void Video::PC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr, bool is4Bpp) {
	dst += y * pitch + x;
//	assert(chr >= 32);
	if(chr < 32)
		return;
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

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr, bool is4Bpp) {
//	emu_printf("Video::MAC_drawStringChar\n");	
	DecodeBuffer buf;
	
	memset(&buf, 0, sizeof(buf));
	buf.ptr = dst;
	buf.w = _w;
	buf.pitch = pitch;
	buf.h = _h;
	buf.x = x * _layerScale;
	buf.y = y * _layerScale;
	
//	emu_printf("Video::drawString('w %d h %d x %d y %d p %d scale%d chr %d)\n", _w,_h,x,y,buf.pitch,_layerScale,chr);
//	if (is4Bpp)
//		buf.setPixel = Video::MAC_setPixelFont4Bpp;
//	else
	buf.setPixel = Video::MAC_setPixelFont;
	_MAC_fontFrontColor = color;
	_MAC_fontShadowColor = _charShadowColor;
//	assert(chr >= 32);
	if(chr<32)
		return;
	_res->MAC_decodeImageData(_res->_fnt, chr - 32, &buf);
}

const char *Video::drawString(const char *str, int16_t x, int16_t y, uint8_t col) {
//	emu_printf("Video::drawString('%s', %d, %d, 0x%X)\n", str, x, y, col);
//	memset4_fast(&_frontLayer[((y<<1)-1)*_w], 0xC0,16*_w);
	const uint8_t *fnt = _res->_fnt;
	int len = 0;
	while (1) {
		const uint8_t c = *str++;
		if (c == 0 || c == 0xB || c == 0xA) {
			break;
		}
		(this->*_drawChar)(_frontLayer, _w, x + len * CHAR_W, y, fnt, col, c, 0);
		++len;
	}
	_stub->copyRect(x, (y<<1), _w, 16, _frontLayer, _w);	
//	markBlockAsDirty(x, y, len * CHAR_W, CHAR_H, _layerScale);
	return str - 1;
}
/*
const char *Video::drawStringSprite(const char *str, int16_t x, int16_t y, uint8_t col) {
//	emu_printf("Video::drawString('%s', %d, %d, 0x%X)\n", str, x, y, col);
	const uint8_t *fnt = _res->_fnt;
	int len = 0;	
	while (1) {
		const uint8_t c = *str++;
		if (c == 0 || c == 0xB || c == 0xA) {
			break;
		}
		(this->*_drawChar)((uint8_t *)_txt1Layer, _w, x + len * CHAR_W*2, y, fnt, col, c, 1);
		++len;
	}
//	markBlockAsDirty(x, y, len * CHAR_W, CHAR_H, _layerScale);
	return str - 1;
}
*/
void Video::drawStringLen(const char *str, int len, int x, int y, uint8_t color) {
	const uint8_t *fnt = _res->_fnt;
	for (int i = 0; i < len; ++i) {
		(this->*_drawChar)(_frontLayer, _w, x + i * CHAR_W, y, fnt, color, str[i], 0);
	}
//	markBlockAsDirty(x, y, len * CHAR_W, CHAR_H , GAMESCREEN_W * GAMESCREEN_H * 4);
}

void Video::MAC_decodeMap(int level, int room) {
	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.ptr = _backLayer;
	buf.w = buf.pitch = _w;
	buf.h = _h;
	buf.setPixel = Video::MAC_setPixel;

	_res->MAC_loadLevelRoom(level, room, &buf);
//	memcpy(_backLayer, _frontLayer, GAMESCREEN_W * GAMESCREEN_H * 4);
//	memset(_frontLayer,0x00, buf.w * buf.h); // ralentit le changement d'écran
	
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
//#ifdef COLOR_4BPP
void Video::MAC_setPixel4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	const int offset2 = (y-buf->y) * (buf->h2>>1) + ((x>>1)-(buf->x>>1));	
	buf->ptr[offset] = color;
	if(x&1)
		buf->ptrsp[offset2] |= (color&0x0f);
	else
	{
		buf->ptrsp[offset2] |= ((color&0x0f)<<4);
	}
}
//#endif
void Video::MAC_setPixel(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	const int offset2 = (y-buf->y) * buf->h2 + (x-buf->x);	
//	buf->ptrbg[x] = color;
	buf->ptr[offset] = color;
	buf->ptrsp[offset2] = color;
}

void Video::MAC_setPixelFG(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	buf->ptr[offset] = color;
}

//#ifdef COLOR_4BPP
void Video::MAC_setPixelMask4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	const int offset2 = (y-buf->y) * (buf->h2>>1) + ((x>>1)-(buf->x>>1));
	if ((buf->ptr[offset] & 0x80) == 0) {
		if(x&1)
			buf->ptrsp[offset2] |= (color&0x0f);
		else
		{
			buf->ptrsp[offset2] |= ((color&0x0f)<<4);
		}
//		buf->ptr[offset] = color;
	}
}
//#endif
void Video::MAC_setPixelMask(DecodeBuffer *buf, int x, int y, uint8_t color) {
	const int offset = y * buf->pitch + x;
	const int offset2 = (y-buf->y) * buf->h2 + (x-buf->x);
	if ((buf->ptr[offset] & 0x80) == 0) {
		buf->ptrsp[offset2] = color;
//		buf->ptr[offset] = color;
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

void Video::MAC_setPixelFont4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color) {

	const int offset2 = y * buf->pitch/2 + x/2;
	uint8_t col;

	switch (color) {
	case 0xC0:
		col = _MAC_fontShadowColor;
		break;
	case 0xC1:
		col = _MAC_fontFrontColor;
		break;
	}

	col&=0x0f;// -= 0xD0;
	
	if(x&1)
		buf->ptr[offset2] |= col;
	else
		buf->ptr[offset2] |= (col<<4);
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


void Video::MAC_drawFG(int x, int y, const uint8_t *data, int frame) {
	const uint8_t *dataPtr = _res->MAC_getImageData(data, frame);
//emu_printf("MAC_drawFG\n");
	if (dataPtr) {
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.w  = buf.pitch = _w;
		buf.h  = _h;
		buf.x  = x * _layerScale;
		buf.y  = y * _layerScale;
		fixOffsetDecodeBuffer(&buf, dataPtr);

		buf.setPixel = MAC_setPixelFG;
		buf.ptr      = _frontLayer;
		_res->MAC_decodeImageData(data, frame, &buf);
	}
}

//extern uint16_t _monster_tex[700];

void Video::MAC_drawSprite(int x, int y, const uint8_t *data, int frame, int anim_number, bool xflip, bool eraseBackground) {
//emu_printf("MAC_drawSprite %p %p\n",data,_res->_monster);	
//emu_printf("MAC_drawSprite w2 %d h2 %d\n",buf.w2,buf.h2);

	DecodeBuffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.xflip = xflip;
	buf.x  = x * _layerScale;
	buf.y  = y * _layerScale;
			
#ifdef PRELOAD_MONSTERS
	if(data!=_res->_monster)
#endif
	{
		const uint8_t *dataPtr = _res->MAC_getImageData(data, frame);

		if (dataPtr) 	{

			buf.w  = buf.pitch = _w;
			buf.w2 = READ_BE_UINT16(dataPtr + 2);
			buf.h  = _h;
			buf.h2 = (READ_BE_UINT16(dataPtr)+7) & ~7;
			fixOffsetDecodeBuffer(&buf, dataPtr);			
		
			if (buf.h2!=352 && buf.h2!=176 && data!=_res->_icn)
				buf.ptr = _backLayer;
			else
				buf.ptr = NULL;			
		
			buf.ptrsp = hwram_screen;
//emu_printf("perso frame %d index %d w %d h %d\n",frame,frame-0x22F,buf.w2,buf.h2);			
			TEXTURE *txptr = &tex_spr[0];
			*txptr = TEXDEF(buf.h2, buf.w2, position_vram);
			memset(buf.ptrsp,0x00,buf.w2*buf.h2);			
#ifdef PRELOAD_MONSTERS			
			if(data==_res->_perso)
#else
			if(data==_res->_perso || data==_res->_monster)	
#endif
				buf.setPixel = eraseBackground ? MAC_setPixel4Bpp : MAC_setPixelMask4Bpp;
			else
				buf.setPixel = eraseBackground ? MAC_setPixel : MAC_setPixelMask;
		
			_res->MAC_decodeImageData(data, frame, &buf);
			
			if(data==_res->_perso)
			{
				memcpy((void *)(SpriteVRAM + ((txptr->CGadr) << 3)),(void *)buf.ptrsp,buf.w2*buf.h2/2);
				position_vram+=(buf.w2*buf.h2)/2;
			}
			else
			{
				memcpy((void *)(SpriteVRAM + ((txptr->CGadr) << 3)),(void *)buf.ptrsp,buf.w2*buf.h2);
				position_vram+=(buf.w2*buf.h2);
			}
			_res->_sprData[anim_number].cgaddr = txptr->CGadr;
			_res->_sprData[anim_number].size   = (buf.h2/8)<<8|buf.w2;			

			SAT_displaySprite(_res->_sprData[anim_number], buf,data);
		}
//emu_printf("frame monster %d index %d w %d h %d\n",frame,frame-0x22F,buf.w2,buf.h2);
	}
	else
	{
		buf.w2 =  _res->_sprData[anim_number].size & 0xFF;
		buf.h2 = (_res->_sprData[anim_number].size>>8)*8;
		if(buf.xflip)
			buf.x  += _res->_sprData[anim_number].x_flip;
		else
			buf.x  -= _res->_sprData[anim_number].x;
	
		buf.y  -= _res->_sprData[anim_number].y;		
		SAT_displaySprite(_res->_sprData[anim_number], buf,data);
	}
}

void Video::SAT_displaySprite(SAT_sprite spr, DecodeBuffer buf, const uint8_t *data)
{
	SPRITE user_sprite;
	user_sprite.CTRL=0;

#ifdef COLOR_4BPP
	user_sprite.PMOD= CL16Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
	
	user_sprite.COLR= 32;
if(data==_res->_monster)
{
	user_sprite.COLR= 80;
#ifdef PRELOAD_MONSTERS	
	user_sprite.CTRL=(buf.xflip?(1 << 4):0);
#endif
}
if(data==_res->_perso)
	user_sprite.COLR= 64;

if(data==_res->_icn)
	user_sprite.COLR= 0xa*16;
#else
	
if(data==_res->_monster)
{
	user_sprite.COLR= 80;	
	user_sprite.CTRL=(buf.xflip?(1 << 4):0);
	user_sprite.PMOD= CL16Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
}
else if(data==_res->_perso)
{
	user_sprite.COLR= 64;	
//	user_sprite.CTRL=(buf.xflip?(1 << 4):0);
	user_sprite.PMOD= CL16Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
}
else
{
	user_sprite.COLR= 0;
	user_sprite.PMOD= CL256Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites	
}
#endif
	if(spr.cgaddr<0x10000)
		user_sprite.SRCA = spr.cgaddr;
	else
	{
		TEXTURE *txptr = &tex_spr[0];
		*txptr = TEXDEF(buf.h2, buf.w2, position_vram);

emu_printf("copy from lwram to vram!!!!!! %p to %p\n",spr.cgaddr,SpriteVRAM + ((txptr->CGadr) << 3));	

		position_vram+=(buf.w2*buf.h2)/2;
	
		memcpy((uint8_t *)(SpriteVRAM + ((txptr->CGadr) << 3)),(void *)spr.cgaddr,(buf.w2*buf.h2)/2);
		user_sprite.SRCA = txptr->CGadr;
	}
	user_sprite.SIZE= spr.size;//(buf.h2/8)<<8|buf.w2;
	user_sprite.XA=63+(buf.x-320);
	user_sprite.YA=(buf.y-224);
	user_sprite.GRDA=0;	
	
	slSetSprite(&user_sprite, toFIXED2(10));	// à remettre // ennemis et objets
}

#ifndef SLAVE_SOUND
void Video::SAT_displayCutscene(unsigned char front, int x, int y, unsigned short h, unsigned short w)
{
	SPRITE user_sprite;

	user_sprite.PMOD=CL256Bnk| ECdis | SPdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
/*
	if(!front)
	{
		user_sprite.SRCA = BACK_RAM_VDP2 / 8;
		memcpy((void *)(SpriteVRAM + BACK_RAM_VDP2),(void *)_res->_scratchBuffer, h*w);
	}
	else
	{
		user_sprite.SRCA=cgaddress8;
		memcpy((void *)(SpriteVRAM + cgaddress),(void *)_res->_scratchBuffer+(IMG_SIZE*1), h*w);		
	}
*/	
	user_sprite.COLR=0;
	user_sprite.SIZE=(w/8)<<8|h;
	user_sprite.CTRL=FUNC_Sprite | _ZmCC;
	user_sprite.XA=x;
	user_sprite.YA=y;

	user_sprite.XB=user_sprite.XA+(w<<1);
	user_sprite.YB=user_sprite.YA+(h<<1);

	user_sprite.GRDA=0;	
//	slSetSprite(&user_sprite, toFIXED2(240));	// à remettre // ennemis et objets
	
	
		user_sprite.SRCA=cgaddress8+(position_vram_aft_monster/8);
		memcpy((void *)(SpriteVRAM + cgaddress + position_vram_aft_monster),(void *)_res->_scratchBuffer+(IMG_SIZE*1), h*w);	
	slSetSprite(&user_sprite, toFIXED2(240));	// à remettre // ennemis et objets	
}
#endif
