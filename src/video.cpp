#pragma GCC optimize ("O2")
#define PRELOAD_MONSTERS 1
#define VRAM_MAX 0x65000 // ne pas toucher
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C"
{
#include <sl_def.h>	
#include "sat_mem_checker.h"
#include "saturn_print.h"
#include <string.h>
extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
extern Uint8 *current_lwram;
extern Uint8 *hwram_screen;
extern Uint8 *current_dram2;
extern bool has4mb;
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

	_layerScale = /*(_res->_type == kResourceTypeMac) ?*/ 2 /*: 1*/; // Macintosh version is 512x448
	_w = GAMESCREEN_W * _layerScale;
	_h = GAMESCREEN_H * _layerScale;

	if(has4mb)
	{
		_frontLayer = (uint8 *)current_dram2;
		current_dram2 += (_w * _h);
	}
	else
	{
		_frontLayer = (uint8_t *)0x2aeff8;//sat_malloc(_w * _h);
	}

	memset(&_frontLayer[0], 0, _w * _h);
	_backLayer = (uint8_t *)VDP2_VRAM_B0;
	memset(_backLayer, 0, _w * _h); // vbt à remettre
	
	_fullRefresh = true;
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
	
//	_stub->updateScreen(0);
//	emu_printf("Video::fullRefresh %d\n",_fullRefresh);	
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
//	{
//		_shakeOffset = 0;
//		_fullRefresh = true;
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

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
    if (chr < 32) return;

    const unsigned char *srcData = src + ((chr - 32) << 8);
    dst += (y * 2) * 512 + (x * 2);

    register const unsigned char front = color;
    register const unsigned char shadow = _charShadowColor;

    for (int i = 0; i < 16; i++, dst += 496) {  // Move dst by (512 - 16) in one step
        for (int j = 0; j < 4; j++) 
		{
			 *dst++ = (*srcData) == 0xC0 ? shadow : (*srcData) == 0xC1 ? front : *dst;
			 srcData++;
			 *dst++ = (*srcData) == 0xC0 ? shadow : (*srcData) == 0xC1 ? front : *dst;
			 srcData++;
			 *dst++ = (*srcData) == 0xC0 ? shadow : (*srcData) == 0xC1 ? front : *dst;
			 srcData++;
			 *dst++ = (*srcData) == 0xC0 ? shadow : (*srcData) == 0xC1 ? front : *dst;
			 srcData++;
        }
    }
}
/*
void Video::MAC_drawStringCharRow(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr, int row) {
    if (chr < 32) return;

    const unsigned char *srcData = src + ((chr - 32) << 8) + (row * 4);
    dst += (y * 512) + (x * 2);

    register const unsigned char front = color;
    register const unsigned char shadow = _charShadowColor;

    for (int col = 0; col < 4; col++) {
        *dst++ = (*srcData == 0xC0) ? shadow : (*srcData == 0xC1) ? front : *dst;
        srcData++;
        *dst++ = (*srcData == 0xC0) ? shadow : (*srcData == 0xC1) ? front : *dst;
        srcData++;
        *dst++ = (*srcData == 0xC0) ? shadow : (*srcData == 0xC1) ? front : *dst;
        srcData++;
        *dst++ = (*srcData == 0xC0) ? shadow : (*srcData == 0xC1) ? front : *dst;
        srcData++;
    }
}
*/
/*

    if (chr < 32) return;

    // Get source data pointer - keeping byte alignment for big-endian
    const uint8_t* srcData = src + ((chr - 32) << 8);
    
    // Calculate base destination pointer
    dst += (uint32_t)(y * _layerScale) * 512 + (x * _layerScale);
    
    // Cache colors and comparisons in registers
    register const uint8_t front = color;
    register const uint8_t shadow = _charShadowColor;
    register const uint8_t shadow_check = 0xC0;
    register const uint8_t front_check = 0xC1;
    
    // Pre-calculate row skip once
    const int rowSkip = 512 - 16;
    
    // Process 16 rows - unroll by 2 for better pipeline usage
    for (int i = 0; i < 16; i += 2) {
        // First row
        #define PROCESS_4_PIXELS(N) \
            register uint8_t p0##N = *srcData++; \
            register uint8_t p1##N = *srcData++; \
            register uint8_t p2##N = *srcData++; \
            register uint8_t p3##N = *srcData++; \
            \
            *dst = (p0##N == shadow_check) ? shadow : \
                  (p0##N == front_check) ? front : *dst; \
            dst++; \
            *dst = (p1##N == shadow_check) ? shadow : \
                  (p1##N == front_check) ? front : *dst; \
            dst++; \
            *dst = (p2##N == shadow_check) ? shadow : \
                  (p2##N == front_check) ? front : *dst; \
            dst++; \
            *dst = (p3##N == shadow_check) ? shadow : \
                  (p3##N == front_check) ? front : *dst; \
            dst++;

        // Process first row (16 pixels in 4 chunks)
        PROCESS_4_PIXELS(0)
        PROCESS_4_PIXELS(1)
        PROCESS_4_PIXELS(2)
        PROCESS_4_PIXELS(3)
        
        dst += rowSkip;

        // Process second row (16 pixels in 4 chunks)
        PROCESS_4_PIXELS(4)
        PROCESS_4_PIXELS(5)
        PROCESS_4_PIXELS(6)
        PROCESS_4_PIXELS(7)
        
        dst += rowSkip;
    }
    #undef PROCESS_4_PIXELS
}
*/
/*
void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
    if (chr < 32) return;

    // Get source data pointer - keeping byte alignment for big-endian
    const uint8_t* srcData = src + ((chr - 32) << 8);
    
    // Calculate base destination pointer
    dst += (uint32_t)(y * _layerScale) * 512 + (x * _layerScale);
    
    // Cache colors in registers
    register const uint8_t front = color;
    register const uint8_t shadow = _charShadowColor;
    
    // Process 16 rows
    for (int i = 0; i < 16; i++) {
        // Process each row in 4-byte chunks (16 pixels = 4 chunks)
        for (int j = 0; j < 4; j++) {
            // In big-endian, bytes are in natural order
            register uint8_t p0 = *srcData++;
            register uint8_t p1 = *srcData++;
            register uint8_t p2 = *srcData++;
            register uint8_t p3 = *srcData++;
            
            // Process 4 pixels sequentially
            *dst = (p0 == 0xC0) ? shadow : 
                  (p0 == 0xC1) ? front : *dst;
            dst++;
            
            *dst = (p1 == 0xC0) ? shadow : 
                  (p1 == 0xC1) ? front : *dst;
            dst++;
            
            *dst = (p2 == 0xC0) ? shadow : 
                  (p2 == 0xC1) ? front : *dst;
            dst++;
            
            *dst = (p3 == 0xC0) ? shadow : 
                  (p3 == 0xC1) ? front : *dst;
            dst++;
        }
        
        // Move to next row
        dst += 512 - 16;
    }
}
*/
/*

void Video::MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr) {
    if (chr < 32) return;

    // Get source data pointer - keeping byte alignment for big-endian
    const uint8_t* srcData = src + ((chr - 32) << 8);
    
    // Calculate base destination pointer
    dst += (uint32_t)(y * _layerScale) * 512 + (x * _layerScale);
    
    // Cache colors in registers
    register const uint8_t front = color;
    register const uint8_t shadow = _charShadowColor;
    
    // Single loop - process all 256 pixels (16x16)
    // Every 16 pixels we need to add pitch-16 to dst
    for (int i = 0; i < 256; i++) {
        register uint8_t pixel = *srcData++;
        
        *dst = (pixel == 0xC0) ? shadow : 
              (pixel == 0xC1) ? front : *dst;
        dst++;
        
        // Move to next row after every 16 pixels
        if ((i & 15) == 15) {
            dst += 512 - 16;
        }
    }
}*/

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
	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();	
	_res->MAC_loadLevelRoom(level, room, &buf);
	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration bg : %d\n",e-s);

	Color roomPalette[512];
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
		if (j == 5 || j == 7 || j == 14 || j == 15) { // utile uniquement si on affiche les textes en sprite
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
    SAT_sprite spriteData;

    // Determine sprite type and index
    bool is_spc = (data == _res->_spc);
    bool is_monster = (data == _res->_monster);
    bool is_special = is_spc && (has4mb || anim_number == 1903 || anim_number == 1560);
    int index = is_spc ? _res->NUM_SPRITES + frame : anim_number;

    // Handle sprite data
    const uint8_t *dataPtr = data;
    if (is_monster || is_special) {
        spriteData = _res->_sprData[index];
        buf.dst_x += (xflip ? -spriteData.x_flip : -spriteData.x);
        if (buf.dst_x >= 512) return;
        buf.dst_w = spriteData.size & 0xFF;
        buf.dst_h = (spriteData.size >> 8) * 8;
        buf.dst_y -= spriteData.y;
        dataPtr = (uint8_t *)spriteData.cgaddr;
    } else {
        dataPtr = _res->MAC_getImageData(data, frame);
        if (!dataPtr) return;
        buf.dst_w = (dataPtr[2] << 8 | dataPtr[3]) & 0xFF; // Inline READ_BE_UINT16
        buf.dst_h = ((dataPtr[0] << 8 | dataPtr[1]) + 7) & ~7;
        buf.ptr = hwram_screen;
        buf.type = (data == _res->_perso) ? 1 : 0;
        spriteData.color = (data == _res->_perso) ? 4 : -1;
        spriteData.size = (buf.dst_h / 8) << 8 | buf.dst_w;
    }

    // Compute data size
    size_t dataSize = buf.dst_w * buf.dst_h;
    if (buf.type == 1) dataSize >>= 1;
    dataSize = (dataSize + 7) & ~7; // Inline SAT_ALIGN

    // Process sprite
    int oldcgaddr = spriteData.cgaddr;
    if (is_monster || is_special) {
        if (spriteData.cgaddr > 0x10000) {
//			emu_printf("decompressed just copy %d spc %d monster %d h %d\n", index, (data == _res->_spc), (data == _res->_monster),buf.dst_h);
            spriteData.cgaddr = SAT_copySpriteToVram((uint8_t *)oldcgaddr, buf, dataSize);
        }
    } else {
        // Clear and decode (optimized clear)
        uint8_t *ptr = buf.ptr;
        for (size_t i = 0; i < dataSize; i++) {
            ptr[i] = 0;
        }
        fixOffsetDecodeBuffer(&buf, dataPtr);
        _res->MAC_decodeImageData(data, frame, &buf, 0xFF);
        spriteData.cgaddr = SAT_copySpriteToVram(buf.ptr, buf, dataSize);
    }

    // Display sprite
//			emu_printf("frame %d cgaddr=%06x %d h %d h not rounded %d\n",frame, spriteData.cgaddr,buf.dst_h, READ_BE_UINT16(dataPtr), buf.dst_h-READ_BE_UINT16(dataPtr));
//			emu_printf("decompression and copy %d spc %d monster %d h %d\n", index, (data == _res->_spc), (data == _res->_monster),buf.dst_h);
    SAT_displaySprite(spriteData, buf);
    spriteData.cgaddr = oldcgaddr;
}

// Helper function to copy sprite to VRAM
uint32_t Video::SAT_copySpriteToVram(void* src, DecodeBuffer &buf, size_t dataSize) {
    if (position_vram + dataSize > VRAM_MAX + 0x19000) {
        position_vram = position_vram_aft_monster;
    }

    TEXTURE tx = TEXDEF(buf.dst_h, buf.dst_w, position_vram);
    DMA_ScuMemCopy((void *)(SpriteVRAM + (tx.CGadr << 3)), src, dataSize);
    position_vram += (dataSize*4)>>2;
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

void Video::SAT_displayCutscene(unsigned char front, int x, int y, unsigned short h, unsigned short w)
{
    SPRITE user_sprite;
    user_sprite.PMOD = CL256Bnk | ECdis | SPdis | 0x0800;
    user_sprite.COLR = 0;
    user_sprite.SIZE = (w / 8) << 8 | h;
    user_sprite.CTRL = FUNC_Sprite | _ZmCC;
    user_sprite.XA = x;
    user_sprite.YA = y;
    user_sprite.XB = x + (w << 1);
    user_sprite.YB = y + (h << 1);
    user_sprite.GRDA = 0;

    const size_t spriteVramOffset = 0x80000 - IMG_SIZE;
//    const uint8_t* bufferOffset = _res->_scratchBuffer + (front ? 0 : IMG_SIZE);
    const uint8_t* bufferOffset = (front ? hwram_screen : _res->_scratchBuffer);
	user_sprite.SRCA = spriteVramOffset / 8;

    memcpy((void *)(SpriteVRAM + spriteVramOffset), bufferOffset, h * w);

    slSetSprite(&user_sprite, toFIXED2(240));	// à remettre // ennemis et objets
}

/*
void Video::SAT_displayCutscene(unsigned char front, int x, int y, unsigned short h, unsigned short w) {
    // Validate dimensions
    size_t size = h * w;
//    if (size > IMG_SIZE || size > VRAM_MAX - (0x80000 - IMG_SIZE)) return;

    SPRITE user_sprite;
    user_sprite.PMOD = CL256Bnk | ECdis | SPdis | 0x0800;
    user_sprite.COLR = 0;
    user_sprite.SIZE = (w / 8) << 8 | h; // w / 8
    user_sprite.CTRL = 0x2000; // FUNC_Sprite | _ZmCC
    user_sprite.XA = x;
    user_sprite.YA = y;
    user_sprite.XB = x + (w << 1);
    user_sprite.YB = y + (h << 1);
    user_sprite.GRDA = 0;
    user_sprite.SRCA = (0x80000 - IMG_SIZE) >> 3; // spriteVramOffset / 8

    uint8_t *dst = (uint8_t *)SpriteVRAM + (0x80000 - IMG_SIZE);
    uint8_t *src = front ? hwram_screen : _res->_scratchBuffer;

    // Manual copy for small transfers, DMA for large
    if (size < 256) {
        for (size_t i = 0; i < size; i++) {
            dst[i] = src[i];
        }
    } else {
        DMA_ScuMemCopy(dst, src, size);
    }

    slSetSprite(&user_sprite, toFIXED2(240)); // Inline toFIXED2(240)
}
*/
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
	slSynch();
}
/*
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
