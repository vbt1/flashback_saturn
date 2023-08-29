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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "intern.h"

struct Resource;
struct SystemStub;

struct Video {
	typedef void (Video::*drawCharFunc)(uint8_t *, int, int, int, const uint8_t *, uint8_t, uint8_t);
	
	enum {
		GAMESCREEN_W = 256,
		GAMESCREEN_H = 224,
		SCREENBLOCK_W = 8,
		SCREENBLOCK_H = 8,
		CHAR_W = 8,
		CHAR_H = 8
	};

	static const uint8_t _conradPal1[];
	static const uint8_t _conradPal2[];
	static const uint8_t _textPal[];
	static const uint8_t _palSlot0xF[];

	Resource *_res;
	SystemStub *_stub;

	int _w, _h;
	int _layerSize;
	int _layerScale; // 1 for Amiga/PC, 2 for Macintosh
	uint8_t _frontLayer[GAMESCREEN_W * GAMESCREEN_H];
	uint8_t _backLayer[GAMESCREEN_W * GAMESCREEN_H];
	uint8_t _tempLayer[GAMESCREEN_W * GAMESCREEN_H];
	uint8_t _tempLayer2[GAMESCREEN_W * GAMESCREEN_H];
	uint8_t _unkPalSlot1, _unkPalSlot2;
	uint8_t _mapPalSlot1, _mapPalSlot2, _mapPalSlot3, _mapPalSlot4;
	uint8_t _charFrontColor;
	uint8_t _charTransparentColor;
	uint8_t _charShadowColor;
	uint8_t _screenBlocks[(GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H)];
	bool _fullRefresh;
	uint8_t _shakeOffset;
	drawCharFunc _drawChar;
	
	Video(Resource *res, SystemStub *stub);
	~Video();

	void markBlockAsDirty(int16_t x, int16_t y, uint16_t w, uint16_t h, int scale);	
	void updateScreen();
	void fullRefresh();
	void fadeOut();
	void setPaletteSlotBE(int palSlot, int palNum);
	void setPaletteSlotLE(int palSlot, const uint8_t *palData);
	void setTextPalette();
	void setPalette0xF();
	void copyLevelMap(uint16 room);
	void decodeLevelMap(uint16 sz, const uint8_t *src, uint8_t *dst);
	void setLevelPalettes();
	void drawSpriteSub1(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void drawSpriteSub2(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void drawSpriteSub3(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void drawSpriteSub4(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void drawSpriteSub5(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void drawSpriteSub6(const uint8_t *src, uint8_t *dst, int pitch, int h, int w, uint8_t colMask);
	void PC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr);
	void MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr);	
	void drawChar(uint8_t c, int16 y, int16 x);
	const char *drawString(const char *str, int16 x, int16 y, uint8_t col);
	void MAC_decodeMap(int level, int room);
	static void MAC_setPixel(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelMask(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelFont(DecodeBuffer *buf, int x, int y, uint8_t color);
	void fillRect(int x, int y, int w, int h, uint8_t color);
	void MAC_drawSprite(int x, int y, const uint8_t *data, int frame, bool xflip, bool eraseBackground);
};

#endif // __VIDEO_H__
