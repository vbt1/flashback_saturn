
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef VIDEO_H__
#define VIDEO_H__

#include "intern.h"
#define	    VDP2_VRAM_B0	0x25e40000
#define	    VDP2_VRAM_B1	0x25e60000

#undef cgaddress
#undef pal
#undef TEXDEF

#define	cgaddress	0x1000 //SpriteBufSize
#define	cgaddress8	cgaddress/8
#define pal1 COL_256
#define TEXDEF(h,v,presize)		{h,v,(cgaddress+(((presize)*4)>>(pal1)))/8,(((h)&0x1f8)<<5 | (v))}
#define IMG_SIZE (240*128)
#define BACK_RAM_VDP2 (cgaddress + IMG_SIZE)
#define AUX_RAM_VDP2  (cgaddress + IMG_SIZE*2)
#define TEXT1_RAM_VDP2 (cgaddress + IMG_SIZE*3)
#define TEXT2_RAM_VDP2 (cgaddress + IMG_SIZE*3+(480*256))

#define	    toFIXED2(a)		((FIXED)(65536.0 * (a)))

struct Resource;
struct SystemStub;

struct Video {
	typedef void (Video::*drawCharFunc)(uint8_t *, int, int, int, const uint8_t *, uint8_t, uint8_t, bool);
	
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
//	int _layerSize;
	int _layerScale; // 1 for Amiga/PC, 2 for Macintosh
	uint8_t *_frontLayer;//[GAMESCREEN_W * GAMESCREEN_H*4];
//	uint8_t _backLayer[GAMESCREEN_W * GAMESCREEN_H*2];
	uint8_t *_backLayer; //, *_txt1Layer, *_txt2Layer;
//	uint8_t _tempLayer[GAMESCREEN_W * GAMESCREEN_H];
//	uint8_t _tempLayer2[GAMESCREEN_W * GAMESCREEN_H];  // à remettre sans x4 (buffer video low res
	uint8_t _unkPalSlot1, _unkPalSlot2;
	uint8_t _mapPalSlot1, _mapPalSlot2, _mapPalSlot3, _mapPalSlot4;
	uint8_t _charFrontColor;
	uint8_t _charTransparentColor;
	uint8_t _charShadowColor;
//	uint8_t *_screenBlocks; //[(GAMESCREEN_W*2 / SCREENBLOCK_W) * (GAMESCREEN_H*2 / SCREENBLOCK_H)];
	bool _fullRefresh;
//	uint8_t _shakeOffset;
	drawCharFunc _drawChar;
	
	Video(Resource *res, SystemStub *stub);
	~Video();

	void markBlockAsDirty(int16_t x, int16_t y, uint16_t w, uint16_t h, int scale);
	void updateScreen();
	void fullRefresh();
	void fadeOut();
	void fadeOutPalette();
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
	void PC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr, bool is4Bpp);
	void MAC_drawStringChar(uint8_t *dst, int pitch, int x, int y, const uint8_t *src, uint8_t color, uint8_t chr, bool is4Bpp);	
	void drawChar(uint8_t c, int16 y, int16 x);
	const char *drawString(const char *str, int16_t x, int16_t y, uint8_t col);
	const char *drawStringSprite(const char *str, int16_t x, int16_t y, uint8_t col);
	void drawStringLen(const char *str, int len, int x, int y, uint8_t color);
	void MAC_decodeMap(int level, int room);
	static void MAC_setPixel(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelFG(DecodeBuffer *buf, int x, int y, uint8_t color);	
	static void MAC_setPixel4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelMask(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelMask4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelFont(DecodeBuffer *buf, int x, int y, uint8_t color);
	static void MAC_setPixelFont4Bpp(DecodeBuffer *buf, int x, int y, uint8_t color);
	void fillRect(int x, int y, int w, int h, uint8_t color);
	void MAC_drawFG(int x, int y, const uint8_t *data, int frame);
	void MAC_drawSprite(int x, int y, const uint8_t *data, int frame, int anim_number, bool xflip, bool eraseBackground);
	void SAT_displayText(int x, int y, unsigned short h, unsigned short w);
	void SAT_displaySprite(SAT_sprite spr, DecodeBuffer buf, const uint8_t *data);
	void SAT_displayCutscene(unsigned char front, int x, int y, unsigned short h, unsigned short w);	
};

#endif // VIDEO_H__
