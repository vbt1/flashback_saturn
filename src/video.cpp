#pragma GCC optimize ("O2")
#define PRELOAD_MONSTERS 1
//#define DEBUG 1
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C"
{
#include <sl_def.h>	
#include "sat_mem_checker.h"
#include <string.h>
extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
extern Uint8 *current_lwram;
extern Uint8 *hwram_screen;
extern Uint8 *hwram_ptr;
}
#include "file.h"
#include "decode_mac.h"
#include "resource.h"
#include "systemstub.h"
#include "video.h"

Video::Video(Resource *res, SystemStub *stub)
	: _res(res), _stub(stub) {

	_layerScale = /*(_res->_type == kResourceTypeMac) ?*/ 2 /*: 1*/; // Macintosh version is 512x448
	_w = GAMESCREEN_W * _layerScale;
	_h = GAMESCREEN_H * _layerScale;
	_layerSize = _w * _h;
	_frontLayer = (uint8_t *)FRONT;

	memset(&_frontLayer[0], 0, _w * _h);
	_backLayer = (uint8_t *)VDP2_VRAM_B0;
	memset(_backLayer, 0, _w * _h); // vbt à remettre
	_fullRefresh = true;
	_shakeOffset = 0;
	_charFrontColor = 0;
	_charTransparentColor = 0;
	_charShadowColor = 0;
	//_drawChar = 0;
/*	switch (_res->_type) {
	case kResourceTypeDOS:
		_drawChar = &Video::PC_drawStringChar;
		break;
	case kResourceTypeMac:*/
//		_drawChar = &Video::MAC_drawStringChar;
/*		break;
	}*/
}

Video::~Video() {
//	sat_free(_frontLayer);
//	sat_free(_backLayer);
//	sat_free(_tempLayer);
//	sat_free(_tempLayer2);
//	sat_free(_screenBlocks);
}

void Video::updateScreen() {
	//	debug(DBG_VIDEO, "Video::updateScreen()");
//	_fullRefresh = false;
	if (_fullRefresh) {
		_stub->copyRect(0, 0, _w, _h, _frontLayer, _w);
		_stub->updateScreen(_shakeOffset);
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
//	{
//		_stub->updateScreen(_shakeOffset);
//		_shakeOffset = 0;
//		_fullRefresh = true; // vbt : on ne recharge pas l'image de l'écran
//	}
}

void Video::fullRefresh() {
//	emu_printf("Video::fullRefresh()\n");	
	_fullRefresh = true;
//	memset(_screenBlocks, 0, (_w / SCREENBLOCK_W) * (_h / SCREENBLOCK_H));
}

void Video::fadeOut() {
//	emu_printf("Video::fadeOut()\n");	
//	if (1) {
		fadeOutPalette();
		slTVOff();
		slSynch();
//	} else {
//		_stub->fadeScreen();
//	}
}

void Video::fadeOutPalette() {
	for (int step = 16; step >= 0; --step) {
		for (int c = 0; c < 512; ++c) {
			Color col;
			_stub->getPaletteEntry(c, &col);
			col.r = col.r * step >> 5;
			col.g = col.g * step >> 5;
			col.b = col.b * step >> 5;
			_stub->setPaletteEntry(c, &col);
		}
//		fullRefresh(); // vbt casse le fadeout des controles
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
*/
void Video::setTextPalette() {
	//	debug(DBG_VIDEO, "Video::setTextPalette()");
	const uint8 *p = _textPal;
	for (int i = 0; i < 16; ++i) { // vbt : à faire plus tard, mettre les 2 dernieres couleurs
		uint16 color = READ_LE_UINT16(p); p += 2;
		Color c;
		c.b = (color & 0x00F) << 2;
		c.g = (color & 0x0F0) >> 2;
		c.r = (color & 0xF00) >> 6;
		if(i < 14)
		{
			_stub->setPaletteEntry(0xE0 + i, &c);  // vbt : front palette
		}
		else
		{
			_stub->setPaletteEntry(68 + i - 14, &c);  // vbt : front palette
		}
		_stub->setPaletteEntry(0x1E0 + i, &c);  // vbt : sprite palette
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

void Video::setIconsPalette() {
	Color clut[512];
//	_res->MAC_copyClut16(clut, 0x1C, 0x37);  // icons
//	_res->MAC_copyClut16(clut, 0x1D, 0x38);
	_res->MAC_copyClutN(clut, 0x1C, 0x37, 32);  // icons

	const int baseColor = 12 * 16 + 256;
	for (int i = 0; i < 32; ++i) {
		int color = baseColor + i;
		_stub->setPaletteEntry(color, &clut[color]);
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
// vbt : meilleure fonction
/*
void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
    if (chr < 32) return;
	
//	emu_printf("MAC_drawStringChar dst %p src %p\n",dst,src);

    const uint8_t *srcData = src + ((chr - 32) << 8);
    dst += (y * 2) * 512 + (x * 2);
    const uint8_t front = color;
    const uint8_t shadow = _charShadowColor;

    for (int i = 0; i < 16; i++, dst += 496) {
        for (int j = 0; j < 4; j++) {
            uint8_t src_val = *srcData++;
            *dst = src_val == 0xC0 ? shadow : (src_val == 0xC1 ? front : *dst);
            dst++;
            src_val = *srcData++;
            *dst = src_val == 0xC0 ? shadow : (src_val == 0xC1 ? front : *dst);
            dst++;
            src_val = *srcData++;
            *dst = src_val == 0xC0 ? shadow : (src_val == 0xC1 ? front : *dst);
            dst++;
            src_val = *srcData++;
            *dst = src_val == 0xC0 ? shadow : (src_val == 0xC1 ? front : *dst);
            dst++;
        }
    }
}
*/
#define CHAR_PIXEL_SIZE 256 // 16x16 pixels, 1 byte per pixel
#define CHAR_OFFSET 32      // ASCII offset for font data
#define SHADOW_PIXEL 0xC0
#define FRONT_PIXEL 0xC1

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *fontData, uint8_t color, uint8_t chr) {
    if (chr < CHAR_OFFSET) return;

    // Compute source and destination pointers
    const uint8_t *srcData = fontData + ((chr - CHAR_OFFSET) * CHAR_PIXEL_SIZE);
    uint8_t *dstPtr = dst + (y * 2 * pitch) + (x * 2);
    const int rowAdvance = pitch; // Full pitch stride per row
    const uint8_t foreground = color;
    const uint8_t shadow = _charShadowColor;

    // Process 16 rows
    for (int i = 0; i < 16; i++) {
        // Process 16 pixels per row in groups of 4
        uint8_t *rowDst = dstPtr;
        const uint8_t *rowSrc = srcData;
        for (int j = 0; j < 16; j += 4) {
            uint8_t src0 = rowSrc[j];
            uint8_t src1 = rowSrc[j + 1];
            uint8_t src2 = rowSrc[j + 2];
            uint8_t src3 = rowSrc[j + 3];
            rowDst[j]     = src0 == SHADOW_PIXEL ? shadow : (src0 == FRONT_PIXEL ? foreground : rowDst[j]);
            rowDst[j + 1] = src1 == SHADOW_PIXEL ? shadow : (src1 == FRONT_PIXEL ? foreground : rowDst[j + 1]);
            rowDst[j + 2] = src2 == SHADOW_PIXEL ? shadow : (src2 == FRONT_PIXEL ? foreground : rowDst[j + 2]);
            rowDst[j + 3] = src3 == SHADOW_PIXEL ? shadow : (src3 == FRONT_PIXEL ? foreground : rowDst[j + 3]);
        }
        dstPtr += rowAdvance;
        srcData += 16;
    }
}
/*
#include <cstdint>
#define CHAR_PIXEL_SIZE 256 // 16x16 pixels, 1 byte per pixel
#define CHAR_OFFSET 32      // ASCII offset for font data
#define SHADOW_PIXEL 0xC0
#define FRONT_PIXEL 0xC1
// nouvelle meilleure version
void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *fontData, uint8_t color, uint8_t chr) {
    if (chr < CHAR_OFFSET) return;

    // Compute source and destination pointers
    const uint8_t *srcData = fontData + ((chr - CHAR_OFFSET) * CHAR_PIXEL_SIZE);
    uint8_t *dstPtr = dst + (y * 2 * pitch) + (x * 2);
    const uint8_t foreground = color;
    const uint8_t shadow = _charShadowColor;
    const int rowAdvance = pitch; // Advance to next row after 16 bytes

    // Process 16 rows
    for (int i = 0; i < 16; i++, dstPtr += rowAdvance, srcData += 16) {
        // Process 16 pixels per row in groups of 4
        for (int j = 0; j < 16; j += 4) {
            uint8_t src0 = srcData[j];
            uint8_t src1 = srcData[j + 1];
            uint8_t src2 = srcData[j + 2];
            uint8_t src3 = srcData[j + 3];

            dstPtr[j]     = src0 == SHADOW_PIXEL ? shadow : (src0 == FRONT_PIXEL ? foreground : dstPtr[j]);
            dstPtr[j + 1] = src1 == SHADOW_PIXEL ? shadow : (src1 == FRONT_PIXEL ? foreground : dstPtr[j + 1]);
            dstPtr[j + 2] = src2 == SHADOW_PIXEL ? shadow : (src2 == FRONT_PIXEL ? foreground : dstPtr[j + 2]);
            dstPtr[j + 3] = src3 == SHADOW_PIXEL ? shadow : (src3 == FRONT_PIXEL ? foreground : dstPtr[j + 3]);
        }
    }
}
*/
/*
#define SCREEN_WIDTH 512
#define CHAR_PIXEL_SIZE 256 // 16x16 pixels, 1 byte per pixel
#define CHAR_OFFSET 32      // ASCII offset for font data
#define SHADOW_PIXEL 0xC0
#define FRONT_PIXEL 0xC1

#include <cstdint>
#include <cassert>
#include <cstdio>


#define SCREEN_WIDTH 512
#define CHAR_PIXEL_SIZE 256 // 16x16 pixels, 1 byte per pixel
#define CHAR_OFFSET 32      // ASCII offset for font data
#define SHADOW_PIXEL 0xC0
#define FRONT_PIXEL 0xC1
#include <cstdint>
#include <cassert>
#include <cstdio>

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *fontData, uint8_t color, uint8_t chr) {
    if (chr < 32) return;
    
    // Use bitshifts for faster multiplication
    const uint8_t *src = fontData + ((chr - CHAR_OFFSET) << 8);
    unsigned short *dstShort = (unsigned short*)(dst + (y << 10) + (x << 1)); // y*1024 + x*2
    
    // Cache colors to avoid repeated member access
    const uint8_t foreground = color;
    const uint8_t shadow = _charShadowColor;
    
    // Process 16 rows
    for (int row = 16; --row >= 0; ) {
        unsigned short *rowPtr = dstShort;
        
        // Process 8 pairs of pixels per row
        for (int col = 8; --col >= 0; ) {
            uint8_t srcVal1 = *src++;
            uint8_t srcVal2 = *src++;
            unsigned short currentPixels = *rowPtr;
            
            // Conditional moves are faster than branches on SH2
            uint8_t pixel1 = (srcVal1 == SHADOW_PIXEL) ? shadow : 
                            (srcVal1 == FRONT_PIXEL) ? foreground : (currentPixels >> 8);
            
            uint8_t pixel2 = (srcVal2 == SHADOW_PIXEL) ? shadow : 
                            (srcVal2 == FRONT_PIXEL) ? foreground : (currentPixels & 0xFF);
            
            *rowPtr++ = (pixel1 << 8) | pixel2;
        }
        
        // Move to next row - use addition instead of complex addressing
        dstShort += 256; // 512 bytes = 256 shorts per row
    }
}
*/
#if 0
void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *fontData, uint8_t color, uint8_t chr) {
    if (chr < 32) return;
    
    // Compute source and destination pointers
    const uint8_t *src = fontData + ((chr - CHAR_OFFSET) << 8);
    unsigned short *dstShort = (unsigned short*)(dst + (y * 2) * 512 + (x * 2));
    
    // Precompute colors
    const uint8_t foreground = color;
    const uint8_t shadow = _charShadowColor;
    
    // Render 16 rows of 16 pixels (8 shorts per row)
    for (int row = 0; row < 16; ++row) {
        // Process 8 pairs of pixels per row
        for (int col = 0; col < 8; ++col) {
            uint8_t srcVal1 = *src++;
            uint8_t srcVal2 = *src++;
            
            // Process first pixel
            uint8_t pixel1 = (srcVal1 == SHADOW_PIXEL) ? shadow : 
                            ((srcVal1 == FRONT_PIXEL) ? foreground : (*dstShort >> 8));
            
            // Process second pixel  
            uint8_t pixel2 = (srcVal2 == SHADOW_PIXEL) ? shadow : 
                            ((srcVal2 == FRONT_PIXEL) ? foreground : (*dstShort & 0xFF));
            
            // Write both pixels as a short in BIG-ENDIAN format
            *dstShort++ = (pixel1 << 8) | pixel2;
        }
        // Move to next row: advance by remaining shorts in row
        dstShort += 248; // 256 - 8 = 248 shorts to next row
    }
}
#endif
const char *Video::drawString(const char *str, int16_t x, int16_t y, uint8_t col) {
//	emu_printf("Video::drawString('%s', %d, %d, 0x%X)\n", str, x, y, col);
	const uint8_t *fnt = _res->_fnt;
	int len = 0;
	while (1) {
		const uint8_t c = *str++;
		if (c == 0 || c == 0xB || c == 0xA) {
			break;
		}
//		(this->*_drawChar)(_frontLayer, _w, x + len * CHAR_W, y, fnt, col, c);
		this->MAC_drawStringChar(_frontLayer, _w, x + len * CHAR_W, y, fnt, col, c);
		++len;
	}
//	_stub->copyRect(x, (y<<1), _w, 16, _frontLayer, _w);	
// vbt à voir s'il faut garder le copyrect
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
		MAC_drawStringChar(_frontLayer, _w, x + i * CHAR_W, y, fnt, color, str[i]);
	}
//	markBlockAsDirty(x, y, len * CHAR_W, CHAR_H , GAMESCREEN_W * GAMESCREEN_H * 4);
}

void Video::MAC_decodeMap(int level, int room) {
	DecodeBuffer buf{};

	buf.ptr = _backLayer;
	buf.dst_w = _w;
	buf.dst_h = _h;

	SAT_cleanSprites(); // vbt : ajout
	slTVOff();
#ifdef DEBUG
	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();	
#endif
	_res->MAC_loadLevelRoom(level, room, &buf);
#ifdef DEBUG
	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration bg : %d\n",e-s);
#endif
	Color roomPalette[512];
//	Color *roomPalette = (Color *)hwram_ptr;
	_res->MAC_setupRoomClut(level, room, roomPalette);

	for (int j = 0; j < 16; ++j) {
		bool specialTextColor = (j == 5 || j == 7 || j == 14 || j == 15);
		int start = specialTextColor ? 14 : 0;
		int end = specialTextColor ? 16 : 16;

		for (int i = start; i < end; ++i) {
			const int color = j * 16 + i;
			_stub->setPaletteEntry(color, &roomPalette[color]);
		}
	}

//vbt ajout pour sprites	
	for (int j = 0; j < 16; ++j) {
		if (j == 5 || j == 7 || j == 14 /*|| j == 15*/) { // utile uniquement si on affiche les textes en sprite
			continue;
		}
		for (int i = 0; i < 16; ++i) {
			const int color = j * 16 + i + 256;
			_stub->setPaletteEntry(color, &roomPalette[color]);
		}
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
    // Cache big-endian reads
    uint16_t offset0 = READ_BE_UINT16(dataPtr + 0); //(dataPtr[0] << 8) | dataPtr[1];   // dataPtr + 0
    uint16_t offset4 = READ_BE_UINT16(dataPtr + 4); //(dataPtr[4] << 8) | dataPtr[5];   // dataPtr + 4
    uint16_t offset6 = READ_BE_UINT16(dataPtr + 6); //(dataPtr[6] << 8) | dataPtr[7];   // dataPtr + 6

    // Update dst_x: xflip ? (offset4 - offset0 - (dst_h - offset0)) : -offset4
    int16_t dx = buf->xflip ? (offset4 - offset0 - (buf->dst_h - offset0)) : -offset4;
    buf->dst_x += dx;

    // Update dst_y
    buf->dst_y -= (int16_t)offset6;
}

void Video::MAC_drawFG(int x, int y, const uint8_t *data, int frame) {
	const uint8_t *dataPtr = _res->MAC_getImageData(data, frame);
//emu_printf("MAC_drawFG %p %d %p\n",data,frame,dataPtr);
	if (dataPtr) {
		DecodeBuffer buf{};
		buf.dst_w  = _w;
		buf.dst_h  = _h;
		buf.dst_x  = x * _layerScale;
		buf.dst_y  = y * _layerScale;

		buf.type	 = 2;
		buf.ptr      = _frontLayer;
		_res->MAC_decodeImageData(data, frame, &buf, 0xff);
	}
}

void Video::MAC_drawSprite(int x, int y, const uint8_t *data, int frame, int anim_number, bool xflip) {
    DecodeBuffer buf = {};
    buf.xflip = xflip;
    buf.dst_x = x * _layerScale;
    buf.dst_y = y * _layerScale;

    // Compute the correct sprite index
    int index = (data == _res->_spc) ? _res->NUM_SPRITES + frame : anim_number;
    bool is_monster = (data == _res->_monster);
    bool is_special = (index == SPR_METRO + _res->NUM_SPRITES || index == SPR_ELEVATOR + _res->NUM_SPRITES);

    // Use reference to make sure changes apply
    SAT_sprite &spriteData = _res->_sprData[index];

    // Handle monster and special sprites
    if (is_monster || is_special) {
        buf.dst_x += (xflip ? -spriteData.x_flip : -spriteData.x);
        if (buf.dst_x >= 512) return;

        buf.dst_w = spriteData.size & 0xFF;
        buf.dst_h = (spriteData.size >> 8) * 8;
        buf.dst_y -= spriteData.y;

        int oldcgaddr = spriteData.cgaddr;

        // Decompressed sprite data is copied to VRAM
        if (spriteData.cgaddr > 0x10000) {
            size_t dataSize = buf.dst_w * buf.dst_h;
            if (spriteData.color != -1) {
                dataSize >>= 1;
            }
            dataSize = (dataSize + 7) & ~7; // Align to 8
            spriteData.cgaddr = SAT_copySpriteToVram((uint8_t *)oldcgaddr, buf, dataSize);
        }

        // Display sprite
        SAT_displaySprite(spriteData, buf);
        spriteData.cgaddr = oldcgaddr;
    } 
    // Handle regular decoded sprites (perso, icn, etc.)
    else {
        const uint8_t *dataPtr = _res->MAC_getImageData(data, frame);
        if (!dataPtr) return;

        buf.dst_w = (dataPtr[2] << 8 | dataPtr[3]) & 0xFF;
        buf.dst_h = ((dataPtr[0] << 8 | dataPtr[1]) + 7) & ~7;
        buf.ptr = hwram_screen;

        size_t dataSize = buf.dst_w * buf.dst_h;
        if (data == _res->_perso) {
            buf.type = 1;
            dataSize >>= 1;
            spriteData.color = 4;
        } else {
            spriteData.color = -1;
        }
        dataSize = (dataSize + 7) & ~7; // Align to 8

        memset(buf.ptr, 0x00, dataSize);
        fixOffsetDecodeBuffer(&buf, dataPtr);
        _res->MAC_decodeImageData(data, frame, &buf, 0xFF);

        spriteData.size = (buf.dst_h / 8) << 8 | buf.dst_w;
        spriteData.cgaddr = SAT_copySpriteToVram(buf.ptr, buf, dataSize);

        // Display sprite
        SAT_displaySprite(spriteData, buf);
    }
}

// Helper function to copy sprite to VRAM
uint32_t Video::SAT_copySpriteToVram(void* src, DecodeBuffer &buf, size_t dataSize) {
    if (position_vram + dataSize > VRAM_MAX + 0x16000) {
        position_vram = position_vram_aft_monster;
    }

    TEXTURE tx = TEXDEF(buf.dst_h, buf.dst_w, position_vram);
    DMA_ScuMemCopy((void *)(SpriteVRAM + (tx.CGadr << 3)), src, dataSize);
    position_vram += (dataSize*4)>>2;
	SCU_DMAWait();
	return tx.CGadr; 
}

void Video::SAT_displaySprite(uint8_t *ptrsp, int x, int y, unsigned short h, unsigned short w)
{
	SPRITE user_sprite;
	user_sprite.CTRL=0;
	user_sprite.COLR = 0xc0;		
	user_sprite.PMOD= CL16Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
	user_sprite.SRCA= ((int)ptrsp)/8;
	user_sprite.SIZE=(w/8)<<8|h;
	user_sprite.XA=63+x;
	user_sprite.YA=y;
	
	slSetSprite(&user_sprite, toFIXED2(10));	// à remettre // ennemis et objets
}

void Video::SAT_displaySprite(SAT_sprite spr, DecodeBuffer buf) {
    // emu_printf("SAT_displaySprite\n");
    SPRITE user_sprite{};
    user_sprite.CTRL = buf.xflip ? (1 << 4) : 0;
    user_sprite.COLR = (spr.color != -1) ? (spr.color<<4) : 0;
    user_sprite.PMOD = (spr.color != -1) ? (CL16Bnk | ECdis | 0x0800) : (CL256Bnk | ECdis | 0x0800);
    user_sprite.SIZE = spr.size;
    user_sprite.XA = 63 + (buf.dst_x - 320);
    user_sprite.YA = buf.dst_y - 224;
    user_sprite.GRDA = 0;
	
    user_sprite.SRCA = spr.cgaddr;
    slSetSprite(&user_sprite, toFIXED2(10)); // à remettre // ennemis et objets
}

void Video::SAT_displayMeshSprite(int x1, int x2, int y1, int y2)
{
	SPRITE user_sprite;

    user_sprite.PMOD = MESHon | SPdis | ECdis | 0x0800;
    user_sprite.CTRL = FUNC_Polygon;
    user_sprite.COLR = 0xCC;
    user_sprite.XA = x1;
    user_sprite.YA = y1+1;
    user_sprite.XB = x2;
    user_sprite.YB = y1+1;

    user_sprite.XD = x1;
    user_sprite.YD = y2-1;
    user_sprite.XC = x2;
    user_sprite.YC = y2-1;
	slSetSprite(&user_sprite, toFIXED2(240));

    user_sprite.PMOD = SPdis | ECdis | 0x0800;
    user_sprite.CTRL = FUNC_Line;
    user_sprite.YA = y1;
    user_sprite.YB = y1;

	slSetSprite(&user_sprite, toFIXED2(240));

    user_sprite.YA = y2;
    user_sprite.YB = y2;

	slSetSprite(&user_sprite, toFIXED2(240));

	if(x2>=239)
		return;

    user_sprite.XA = x2;
    user_sprite.YA = y1;
    user_sprite.XB = x2;

	slSetSprite(&user_sprite, toFIXED2(240));
}

void Video::SAT_cleanSprites()
{
	SPRITE user_sprite;
	user_sprite.CTRL= FUNC_End;
	user_sprite.PMOD=0;
	user_sprite.SRCA=0;
	user_sprite.COLR=0;

	user_sprite.SIZE=0;
	user_sprite.XA=0;
	user_sprite.YA=0;

	user_sprite.XB=0;
	user_sprite.YB=0;
	user_sprite.GRDA=0;

	slSetSprite(&user_sprite, toFIXED2(240));	// à remettre // ennemis et objets
	slSynch(); // vbt à remettre
}

void Video::convert_8bpp_to_4bpp_inplace(uint8_t *buffer, size_t pixel_count) {
    // Process two pixels at a time, overwriting the input
    // Process two pixels at a time, overwriting the input
    for (size_t i = 0; i < pixel_count; i += 2) {
		uint8_t hi = buffer[i];
		uint8_t lo = buffer[i + 1] & 0xF;
		buffer[i>>1] = (hi << 4) | lo;
	}
}
/*
void Video::SAT_displaySpritesPalette()
{
	SPRITE user_sprite;

    size_t spriteVramOffset;
	spriteVramOffset = 0x80000 - 256*64;

    user_sprite.PMOD = CL256Bnk | ECdis | SPdis | 0x0800;
    user_sprite.COLR = 0;
    user_sprite.SIZE=(256/8)<<8|64;
    user_sprite.CTRL = 0;
    user_sprite.XA = -256;
    user_sprite.YA = -224;
    user_sprite.GRDA = 0;
    user_sprite.SRCA = spriteVramOffset / 8;

	unsigned int k = 0;

	for (int j = 0; j < 8; j++) 
	{
		unsigned char* vram = (unsigned char*)SpriteVRAM + spriteVramOffset + j * 8 * 256;  // Start position for this iteration

		for (int i = 0; i < 32; i++) {
			unsigned char value = i + k;

			for (int n = 0; n < 8; n++) {
				memset(vram, value, 8);  // Set 8 bytes at once
				vram += 256;  // Move to the next line
			}

			vram -= (8 * 256 - 8);  // Adjust position for the next block of 8 lines
		}

		k += 32;
	}
	slSetSprite(&user_sprite, 130<<16);	// fps	
}

void Video::SAT_displayPalette()
{
	unsigned char* vram_base = (unsigned char*)VDP2_VRAM_A0 + 0x4800;
	unsigned int k = 0;

	for (int j = 0; j < 4; j++) {
		unsigned char* vram = vram_base + j * 8 * 512;  // Start position for this iteration

		for (int i = 0; i < 64; i++) {
			unsigned char value = i + k;

			for (int n = 0; n < 8; n++) {
				memset(vram, value, 8);  // Set 8 bytes at once
				vram += 512;  // Move to the next line
			}

			vram -= (8 * 512 - 8);  // Adjust position for the next block of 8 lines
		}

		k += 64;
	}
	_stub->copyRect(0, 20, _w, 16, _frontLayer, _w);
	memset4_fast(&_frontLayer[40 * _w], 0x00, _w * _h);
}
*/
