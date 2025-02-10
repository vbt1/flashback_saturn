
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
//#define DEBUG_CUTSCENE 1
//#define SUBTITLE_SPRITE 1
#define TVSTAT      (*(volatile Uint16 *)0x25F80004)
extern "C"
{
#include <sl_def.h>	
#include <string.h>	
#include "pcm.h"
#include <sega_gfs.h>
#include <gfs_def.h>
#include "gfs_wrap.h"
extern Uint8 *hwram_screen;
extern Uint8 *current_lwram;
extern Uint8 *save_current_lwram;
extern Uint8 frame_x;
void *memset4_fast(void *, long, size_t);
Uint8 transferAux=0;
extern Uint32 gfsLibWork[GFS_WORK_SIZE(OPEN_MAX)/sizeof(Uint32)];
}
#include "mod_player.h"
#include "resource.h"
#include "systemstub.h"
#include "cutscene.h"
#include "saturn_print.h"
#include "video.h"

extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
//extern volatile Uint8 audioEnabled;

int previousLen=0;
uint8_t *previousP=NULL;
int sameP = false;
int sameText = false;

/*
static void scalePoints(Point *pt, int count, int scale) {
//	scale = 1; //vbt force la valeur
	return;
	if (scale != 1) {
		while (count-- > 0) {
			pt->x *= scale;
			pt->y *= scale;
			++pt;
		}
	}
}
*/
Cutscene::Cutscene(Resource *res, SystemStub *stub, Video *vid)
	: _mix(stub), _res(res), _stub(stub), _vid(vid) {
//	_patchedOffsetsTable = 0;
	memset(_palBuf, 0, sizeof(_palBuf));
}

const uint8_t *Cutscene::getCommandData() const {
	return _res->_cmd;
}

const uint8_t *Cutscene::getPolygonData() const {
	return _res->_pol;
}

void Cutscene::sync(int frameDelay) {
	if (_stub->_pi.quit) {
		return;
	}
	if (_stub->_pi.dbgMask & PlayerInput::DF_FASTMODE) {
		return;
	}

	uint8_t frameHz = ((TVSTAT & 1) == 0)?60:50;
	const int32_t delay = _stub->getTimeStamp() - _tstamp;
	const int32_t pause = frameDelay * (1000 / frameHz) - delay;
	if (pause > 0) {
		_stub->sleep(pause);
	}
	//else
/*	if (pause<=0)
		emu_printf("too slow !! duration %d real duration %d delay %d \n",frameDelay * (1000 / frameHz), pause, delay);
	else
		_stub->sleep(pause);
*/
	_tstamp = _stub->getTimeStamp();
}

void Cutscene::copyPalette(const uint8_t *palo, uint16_t num) {
	uint8_t *dst = _palBuf;
	if (num != 0) {
		dst += 0x20;
	}
	memcpy(dst, palo, 0x20);
	_newPal = true;
}

void Cutscene::updatePalette() {
	if (_newPal) {
		const uint8_t *p = _palBuf;
		for (int i = 0; i < 32; ++i) {
			const uint16_t color = READ_BE_UINT16(p); p += 2;
			uint8 t = (color == 0) ? 0 : 3;
			Color c;
// vbt correction des couleurs			
			c.r = ((color & 0xF00) >> 6) | t;
			c.g = ((color & 0x0F0) >> 2) | t;
			c.b = ((color & 0x00F) << 2) | t;
			_stub->setPaletteEntry(0x1C0 + i, &c);
		}
		_newPal = false;
	}
}
struct DecodeBuffer {
	uint8_t *ptr;
	int w, h, pitch;
	int x, y;
	bool xflip;
	uint8_t type;

	void (*setPixel)(DecodeBuffer *buf, int x, int y, uint8_t color);
//	void *dataPtr;
};
void Cutscene::updateScreen() {
//--------------------------
DecodeBuffer buf{};

	if(hasText && !sameText)
	{
		_stub->copyRect(16, 96, 480, _vid->_h-128, _vid->_frontLayer, _vid->_w);
		memset(&_vid->_frontLayer[96*_vid->_w],0,_vid->_w* 320); // nettoyeur de texte
		hasText = false;
	}

	sync(_frameDelay - 1);
	SWAP(_frontPage, _backPage);

    SPRITE user_sprite;
    user_sprite.PMOD = CL16Bnk | ECdis | 0x0800;
    user_sprite.COLR = 0xC0;
    user_sprite.SIZE = (240 / 8) << 8 | 128;

    user_sprite.CTRL = FUNC_Sprite | _ZmCC;
    user_sprite.XA = 0;
    user_sprite.YA = 0;
    user_sprite.XB = 0 + (240 << 1);
    user_sprite.YB = 0 + (128 << 1);
	
    user_sprite.GRDA = 0;
	size_t spriteVramOffset = 0x80000 - (IMG_SIZE/2);

	if (transferAux)
	{
		slTransferEntry((void*)_auxPage, (void*)(SpriteVRAM + spriteVramOffset), 240 * 64);
	}
	user_sprite.SRCA = spriteVramOffset / 8;
    slSetSprite(&user_sprite, toFIXED2(240));	// à remettre // ennemis et objets

	user_sprite.COLR = 0xD0; // vbt mauvaise palette
	spriteVramOffset = 0x80000 - IMG_SIZE - ((_frontPage==_res->_scratchBuffer)? (IMG_SIZE/2):0);
	user_sprite.SRCA = spriteVramOffset / 8;

    unsigned int i;
    uint8_t *aux = (uint8_t *)(SpriteVRAM + spriteVramOffset);  // Use pointers to avoid array indexing overhead
    uint8_t *back = (uint8_t *)_frontPage;
    
	// Unroll the loop by processing 8 elements at a time
	for (i = 0; i < IMG_SIZE; i += 16) {
		uint8_t *b = back + i;
		uint8_t *a = aux + (i / 2);

		// Process 8 pairs of bytes
		a[0] = (b[1]) | (b[0] << 4);
		a[1] = (b[3]) | (b[2] << 4);
		a[2] = (b[5]) | (b[4] << 4);
		a[3] = (b[7]) | (b[6] << 4);
		a[4] = (b[9]) | (b[8] << 4);
		a[5] = (b[11]) | (b[10] << 4);
		a[6] = (b[13]) | (b[12] << 4);
		a[7] = (b[15]) | (b[14] << 4);
	}
    slSetSprite(&user_sprite, toFIXED2(240));
	frame_x++;
	slSynch();
	updatePalette();

	transferAux=0;

#ifdef DEBUG_CUTSCENE
 #define SAT_ALIGN(a) ((a+3)&~3)
	buf.w=240;
	buf.h=128;
	position_vram = 0x1000;
	size_t dataSize = SAT_ALIGN(buf.w * buf.h);
    int cgaddr1 = _vid->SAT_copySpriteToVram((uint8_t *)_backPage, buf, dataSize);
	
    uint8_t *aux2 = (uint8_t *)(SpriteVRAM + cgaddr1*8);  // Use pointers to avoid array indexing overhead
    uint8_t *back2 = (uint8_t *)_backPage;
    
	// Unroll the loop by processing 8 elements at a time
	for (i = 0; i < IMG_SIZE; i += 16) {
		uint8_t *b = back2 + i;
		uint8_t *a = aux2 + (i / 2);

		// Process 8 pairs of bytes
		a[0] = (b[1]) | (b[0] << 4);
		a[1] = (b[3]) | (b[2] << 4);
		a[2] = (b[5]) | (b[4] << 4);
		a[3] = (b[7]) | (b[6] << 4);
		a[4] = (b[9]) | (b[8] << 4);
		a[5] = (b[11]) | (b[10] << 4);
		a[6] = (b[13]) | (b[12] << 4);
		a[7] = (b[15]) | (b[14] << 4);
	}	

	_vid->SAT_displaySprite((uint8_t *)(cgaddr1*8),-320,-224, 128, 240);
//	SAT_displaySprite(spriteData, buf, data);

	int cgaddr3 = _vid->SAT_copySpriteToVram((uint8_t *)_frontPage, buf, dataSize);

    uint8_t *aux1 = (uint8_t *)(SpriteVRAM + cgaddr3*8);  // Use pointers to avoid array indexing overhead
    uint8_t *back1 = (uint8_t *)_frontPage;
    
	// Unroll the loop by processing 8 elements at a time
	for (i = 0; i < IMG_SIZE; i += 16) {
		uint8_t *b = back1 + i;
		uint8_t *a = aux1 + (i / 2);

		// Process 8 pairs of bytes
		a[0] = (b[1]) | (b[0] << 4);
		a[1] = (b[3]) | (b[2] << 4);
		a[2] = (b[5]) | (b[4] << 4);
		a[3] = (b[7]) | (b[6] << 4);
		a[4] = (b[9]) | (b[8] << 4);
		a[5] = (b[11]) | (b[10] << 4);
		a[6] = (b[13]) | (b[12] << 4);
		a[7] = (b[15]) | (b[14] << 4);
	}



	_vid->SAT_displaySprite((uint8_t *)(cgaddr3*8),-320,-96, 128, 240);	

int cgaddr2 = _vid->SAT_copySpriteToVram((uint8_t *)_auxPage, buf, dataSize);
	_vid->SAT_displaySprite((uint8_t *)(cgaddr2*8),-320,32, 128, 240);
	
position_vram = 0x1000;
#endif

}

#if 0
#define SIN(a) (int16_t)(sin(a * M_PI / 180) * 256)
#define COS(a) (int16_t)(cos(a * M_PI / 180) * 256)
#else
#define SIN(a) _sinTable[a]
#define COS(a) _cosTable[a]
#endif

/*
  cos(60)  table: 128, math: 127
  cos(120) table:-127, math:-128
  cos(240) table:-128, math:-127
  sin(330) table: 221, math:-127
*/

/*
  a = rotation angle
  b = scale/distort vertically (180)
  c = scale/distort horizontally (90)

  | x | cos_a    sin_a | cos_b | cos_c * sin_b |
  | y | sin_a   -cos_a | sin_c |             1 |
*/

void Cutscene::setRotationTransform(uint16_t a, uint16_t b, uint16_t c) { // identity a:0 b:180 c:90
	const int16_t sin_a = SIN(a);
	const int16_t cos_a = COS(a);
	const int16_t sin_c = SIN(c);
	const int16_t cos_c = COS(c);
	const int16_t sin_b = SIN(b);
	const int16_t cos_b = COS(b);
	_rotMat[0] = ((cos_a * cos_b) >> 8) - ((((cos_c * sin_a) >> 8) * sin_b) >> 8);
	_rotMat[1] = ((sin_a * cos_b) >> 8) + ((((cos_c * cos_a) >> 8) * sin_b) >> 8);
	_rotMat[2] = ( sin_c * sin_a) >> 8;
	_rotMat[3] = (-sin_c * cos_a) >> 8;
}

static uint8_t getNewLineChar(const Resource *res) {
	return /*(res->_lang == LANG_JP) ? 0xD1 :*/ '|';
}

uint16_t Cutscene::findTextSeparators(const uint8_t *p, int len) {
	const uint8_t newLineChar = getNewLineChar(_res);
	uint8_t *q = _textSep;
	uint16_t ret = 0;
	uint16_t pos = 0;
	for (int i = 0; i < len && p[i] != 0xA; ++i) {
		if (p[i] == newLineChar) {
			*q++ = pos;
			if (pos > ret) {
				ret = pos;
			}
			pos = 0;
		} else {
			++pos;
		}
	}
	*q++ = pos;
	if (pos > ret) {
		ret = pos;
	}
	*q++ = 0;
	return ret;
}

void Cutscene::drawText(int16_t x, int16_t y, const uint8_t *p, uint16_t color, uint8_t *page, int textJustify) {
//emu_printf("Cutscene::drawText(x=%d, y=%d, c=%d, justify=%d)\n", x, y, color, textJustify);
//	memset(&_vid->_frontLayer[y*_vid->_w],0x00,512*(440-y)); // vbt : à voir
	int len = 0;
	int sameP = (previousP == p);
	previousP = (uint8_t *)p;
	if (p != _textBuf /*&& _res->isMac()*/) {
		len = *p++;
	} else {
		while (p[len] != 0xA && p[len]) {
			++len;
		}
	}

	if(previousLen==len && sameP)
	{
		sameText = 1;
//		emu_printf("same text no need to read it\n");
		return;
	}
	previousLen = len;
	sameText = false;
	
//emu_printf("Cutscene::drawText(x=%d, y=%d, c=%d, len=%d)\n", x, y, color, len);
	
//	Video::drawCharFunc dcf = _vid->_drawChar;
	const uint8_t *fnt = /*(_res->_lang == LANG_JP) ? Video::_font8Jp :*/ _res->_fnt;
	uint16_t lastSep = 0;
	if (textJustify != kTextJustifyLeft) {
		lastSep = findTextSeparators(p, len);
		if (textJustify != kTextJustifyCenter) {
			lastSep = /*(_res->_lang == LANG_JP) ? 20 :*/ 30;
		}
	}
	const uint8_t *sep = _textSep;
	y += 50;
	x += /*(_res->_lang == LANG_JP) ? 0 :*/ 8;
	int16_t yPos = y;
	int16_t xPos = x;
	if (textJustify != kTextJustifyLeft) {
		xPos += ((lastSep - *sep++) / 2) * Video::CHAR_W;
	}
	const uint8_t newLineChar = getNewLineChar(_res);
	for (int i = 0; i < len && p[i] != 0xA; ++i) {
		if (p[i] == newLineChar) {
			yPos += Video::CHAR_H;
			xPos = x;
			if (textJustify != kTextJustifyLeft) {
				xPos += ((lastSep - *sep++) / 2) * Video::CHAR_W;
			}
		} else if (p[i] == 0x20) {
			xPos += Video::CHAR_W;
		} else if (p[i] == 0x9) {
			// ignore tab
		} else
		{
//			(_vid->*dcf)(page, _vid->_w, xPos, yPos, fnt, color, p[i]);
			_vid->MAC_drawStringChar(page, _vid->_w, xPos, yPos, fnt, color, p[i]);
			xPos += Video::CHAR_W;
		}
	}
}

void Cutscene::clearBackPage() {
//	emu_printf("_clearScreen %d\n",_clearScreen);
//	if(_clearScreen)
//		memset(&_vid->_frontLayer[96*_vid->_w],0,_vid->_w* 256); // nettoyeur de texte
	memset4_fast(_backPage, 0, IMG_SIZE);
}

void Cutscene::drawCreditsText() {
//emu_printf("drawCreditsText\n");
//	_stub->initTimeStamp();
	if (_creditsKeepText) {
		if (_creditsSlowText) {
			return;
		}
		_creditsKeepText = false;
	}
	if (_creditsTextCounter <= 0) {
		uint8_t code;
//		const bool isMac = _res->isMac();
		if (/*isMac &&*/ _creditsTextLen <= 0) {
			const uint8_t *p = _res->getCreditsString(_creditsTextIndex++);
			if (!p) {
//				emu_printf("no text\n");
				return;
			}
			_creditsTextCounter = 60;
			_creditsTextPosX = p[0]+1;
			_creditsTextPosY = p[1];
			_creditsTextLen = p[2];
			_textCurPtr = p + 2;
			code = 0;
		} else {
			code = *_textCurPtr;
		}
		if (code == 0x7D /*&& isMac*/) {
			++_textCurPtr;
			code = *_textCurPtr++;
			_creditsTextLen -= 2;
//			assert(code > 0x30);
			if(code <= 0x30)
				return;
			for (int i = 0; i < (code - 0x30); ++i) {
				*_textCurBuf++ = ' ';
			}
			*_textCurBuf = 0xA;
		} else if (code == 0xFF) {
			_textBuf[0] = 0xA;
		} else if (code == 0xFE) {
			++_textCurPtr;
			_creditsTextCounter = *_textCurPtr++;
		} else if (code == 1) {
			++_textCurPtr;
			_creditsTextPosX = *_textCurPtr++;
			_creditsTextPosY = *_textCurPtr++;
		} else if (code == 0) {
			_textCurBuf = _textBuf;
			_textBuf[0] = 0xA;
			++_textCurPtr;
			if (_creditsSlowText) {
				_creditsKeepText = true;
			}
		} else {
			*_textCurBuf++ = code;
			*_textCurBuf = 0xA;
			++_textCurPtr;
//			if (isMac) 
				{
				--_creditsTextLen;
				if (_creditsTextLen == 0) {
					_creditsTextCounter = 600;
				}
			}
		}
	} else {
		_creditsTextCounter -= 10;
	}
	hasText = true;
	unsigned int s = _stub->getTimeStamp();	
	drawText((_creditsTextPosX - 1) * 8, _creditsTextPosY * 8, _textBuf, 0xEF, _vid->_frontLayer, kTextJustifyLeft);
	unsigned int e = _stub->getTimeStamp();
//	if(e-s!=0)
//	emu_printf("--duration drawCreditsText : %d\n",e-s);	
}

void Cutscene::op_markCurPos() {
	//emu_printf("Cutscene::op_markCurPos()\n");
	_cmdPtrBak = _cmdPtr;
	_frameDelay = 5;
	if (!_creditsSequence) {
		if (_id == kCineDebut) {
			_frameDelay = 7;
		} else if (_id == kCineChute) {
			_frameDelay = 6;
		}
	} else {
		drawCreditsText();
	}
	updateScreen();
	clearBackPage();
	_creditsSlowText = false;
}

void Cutscene::op_refreshScreen() {
	_clearScreen = fetchNextCmdByte();
	if (_clearScreen != 0) {
		clearBackPage();
		_creditsSlowText = false;
	}
}

void Cutscene::op_waitForSync() {
//	emu_printf("Cutscene::op_waitForSync()\n");
	if (_creditsSequence) {
		uint16_t n = fetchNextCmdByte() * 2;
		do {
			_creditsSlowText = true;
			_frameDelay = 3;
			if (_textBuf == _textCurBuf) {
				_creditsTextCounter = /*_res->isDOS() ? 20 :*/ 60;
			}
			drawCreditsText();
//			updateScreen();
		} while (--n);
//		clearBackPage();
		_creditsSlowText = false;
	} else {
		_frameDelay = fetchNextCmdByte() * 4;
		sync(_frameDelay);
		if(_id != 17)
		{
//			emu_printf("copy front1\n");
			_stub->copyRect(16, 96, 480, _vid->_h-128, _vid->_frontLayer, _vid->_w);
		}
	}
}

void Cutscene::drawShape(const uint8_t *data, int16_t x, int16_t y) {
//emu_printf("Cutscene::drawShape()1\n");
	_gfx.setLayer(_backPage, _vid->_w);
	uint8_t numVertices = *data++;
	if (numVertices & 0x80) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		int16_t rx = READ_BE_UINT16(data); data += 2;
		int16_t ry = READ_BE_UINT16(data); data += 2;
		//scalePoints(&pt, 1, _vid->_layerScale);
		_gfx.drawEllipse(_primitiveColor, _hasAlphaColor, &pt, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		//scalePoints(&pt, 1, _vid->_layerScale);
		_gfx.drawPoint(_primitiveColor, &pt);
	} else {
		Point *pt = _vertices;
		int16_t ix = READ_BE_UINT16(data); data += 2;
		int16_t iy = READ_BE_UINT16(data); data += 2;
		pt->x = ix + x;
		pt->y = iy + y;
		++pt;
		int16_t n = numVertices - 1;
		++numVertices;
		for (; n >= 0; --n) {
			int16_t dx = (int8_t)*data++;
			int16_t dy = (int8_t)*data++;
			if (dy == 0 && n != 0 && *(data + 1) == 0) {
				ix += dx;
				--numVertices;
			} else {
				ix += dx;
				iy += dy;
				pt->x = ix + x;
				pt->y = iy + y;
				++pt;
			}
		}
		//scalePoints(_vertices, numVertices, _vid->_layerScale);
		_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices);
	}
}

void Cutscene::op_drawShape() {
//emu_printf("Cutscene::op_drawShape()2 %d\n",_clearScreen);

	int16_t x = 0;
	int16_t y = 0;
	uint16_t shapeOffset = fetchNextCmdWord();
	if (shapeOffset & 0x8000) {
		x = fetchNextCmdWord();
		y = fetchNextCmdWord();
	}

	const uint8_t *shapeOffsetTable    = _polPtr + READ_BE_UINT16(_polPtr + 0x02);
	const uint8_t *shapeDataTable      = _polPtr + READ_BE_UINT16(_polPtr + 0x0E);
	const uint8_t *verticesOffsetTable = _polPtr + READ_BE_UINT16(_polPtr + 0x0A);
	const uint8_t *verticesDataTable   = _polPtr + READ_BE_UINT16(_polPtr + 0x12);

	const uint8_t *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16_t primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	while (primitiveCount--) {
		uint16_t verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
		const uint8_t *primitiveVertices = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
		int16_t dx = 0;
		int16_t dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData); shapeData += 2;
			dy = READ_BE_UINT16(shapeData); shapeData += 2;
		}
		_hasAlphaColor = (verticesOffset & 0x4000) != 0;
		uint8_t color = *shapeData++;
		/*if (_clearScreen == 0) {
			color += 0x10;
		}*/
		_primitiveColor = color;// & 0x0f;
		drawShape(primitiveVertices, x + dx, y + dy);
	}
	if (_clearScreen != 0) {
//emu_printf("copy bg in aux\n");
		unsigned int i;
		uint8_t *back = _backPage;  // Use pointers to avoid array indexing overhead
		uint8_t *aux = _auxPage;
		
		// Unroll the loop by processing 8 elements at a time
		for (i = 0; i < IMG_SIZE; i += 16) {
			// Process 8 pairs of bytes
			aux[i / 2]       = (back[i + 1]) | (back[i] << 4);
			aux[(i / 2) + 1] = (back[i + 3]) | (back[i + 2] << 4);
			aux[(i / 2) + 2] = (back[i + 5]) | (back[i + 4] << 4);
			aux[(i / 2) + 3] = (back[i + 7]) | (back[i + 6] << 4);
			aux[(i / 2) + 4] = (back[i + 9]) | (back[i + 8] << 4);
			aux[(i / 2) + 5] = (back[i + 11]) | (back[i + 10] << 4);
			aux[(i / 2) + 6] = (back[i + 13]) | (back[i + 12] << 4);
			aux[(i / 2) + 7] = (back[i + 15]) | (back[i + 14] << 4);
		}
		clearBackPage();
		transferAux=1;
	}
}

static int _paletteNum = -1;

void Cutscene::op_setPalette() {
//	emu_printf("Cutscene::op_setPalette()\n");
	uint8_t num = fetchNextCmdByte();
	uint8_t palNum = fetchNextCmdByte();
	uint16_t off = READ_BE_UINT16(_polPtr + 6);
	const uint8_t *p = _polPtr + off + num * 32;
	copyPalette(p, palNum ^ 1);
	if (_creditsSequence) {
		_palBuf[0x20] = 0x0F;
		_palBuf[0x21] = 0xFF;
	}
	_paletteNum = num;
}

void Cutscene::op_drawCaptionText() {
//emu_printf("Cutscene::op_drawCaptionText()\n");
	uint16_t strId = fetchNextCmdWord();
	if (!_creditsSequence) {

//		const int h = 128;
		if (strId != 0xFFFF) {
			const uint8_t *str = _res->getCineString(strId);
			if (str) {
				hasText = true;
//				sameText = false;
				previousP = NULL;
				drawText(0, 129, str, 0xEF, _vid->_frontLayer, kTextJustifyAlign);
			}
		} else if (_id == kCineEspions) {
			// cutscene relies on drawCaptionText opcodes for timing
			_frameDelay = 100;
			sync(_frameDelay);
		}
	}
}

void Cutscene::op_nop() {
//	debug(DBG_CUT, "Cutscene::op_nop()");
}

void Cutscene::op_skip3() {
//	debug(DBG_CUT, "Cutscene::op_skip3()");
	_cmdPtr += 3;
}

void Cutscene::op_refreshAll() {
//	emu_printf("Cutscene::op_refreshAll()\n");
	_frameDelay = 5;
	updateScreen();
	clearBackPage();
//	memset(&_vid->_frontLayer[96*_vid->_w],0,_vid->_w* 256); // nettoyeur de texte
	_creditsSlowText = true;
	op_handleKeys();
}

void Cutscene::drawShapeScale(const uint8_t *data, int16_t zoom, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g) {
//	//emu_printf("Cutscene::drawShapeScale(%d, %d, %d, %d, %d, %d, %d)\n", zoom, b, c, d, e, f, g);
	_gfx.setLayer(_backPage, _vid->_w);
	uint8_t numVertices = *data++;
	if (numVertices & 0x80) {
		int16_t x, y;
		Point *pt = _vertices;
		Point pr[2];
		_shape_cur_x = b + READ_BE_UINT16(data); data += 2;
		_shape_cur_y = c + READ_BE_UINT16(data); data += 2;
		x = READ_BE_UINT16(data); data += 2;
		y = READ_BE_UINT16(data); data += 2;
		_shape_cur_x16 = 0;
		_shape_cur_y16 = 0;
		pr[0].x =  0;
		pr[0].y = -y;
		pr[1].x = -x;
		pr[1].y =  y;
		if (_shape_count == 0) {
			f -= ((((_shape_ix - _shape_ox) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((_shape_iy - _shape_oy) * zoom) * 128) + 0x8000) >> 16;
			pt->x = f;
			pt->y = g;
			++pt;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
		} else {
			_shape_cur_x16 = _shape_prev_x16 + ((_shape_cur_x - _shape_prev_x) * zoom) * 128;
			pt->x = (_shape_cur_x16 + 0x8000) >> 16;
			_shape_cur_y16 = _shape_prev_y16 + ((_shape_cur_y - _shape_prev_y) * zoom) * 128;
			pt->y = (_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		for (int i = 0; i < 2; ++i) {
			_shape_cur_x += pr[i].x;
			_shape_cur_x16 += pr[i].x * zoom * 128;
			pt->x = (_shape_cur_x16 + 0x8000) >> 16;
			_shape_cur_y += pr[i].y;
			_shape_cur_y16 += pr[i].y * zoom * 128;
			pt->y = (_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		Point po;
		po.x = _vertices[0].x + d + _shape_ix;
		po.y = _vertices[0].y + e + _shape_iy;
		int16_t rx = _vertices[0].x - _vertices[2].x;
		int16_t ry = _vertices[0].y - _vertices[1].y;
		//scalePoints(&po, 1, _vid->_layerScale);
		_gfx.drawEllipse(_primitiveColor, _hasAlphaColor, &po, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
 		pt.x = _shape_cur_x = b + READ_BE_UINT16(data); data += 2;
	 	pt.y = _shape_cur_y = c + READ_BE_UINT16(data); data += 2;
 		if (_shape_count == 0) {
			f -= ((((_shape_ix - pt.x) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((_shape_iy - pt.y) * zoom) * 128) + 0x8000) >> 16;
			pt.x = f + _shape_ix + d;
			pt.y = g + _shape_iy + e;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
		} else {
			_shape_cur_x16 = _shape_prev_x16 + ((pt.x - _shape_prev_x) * zoom) * 128;
			_shape_cur_y16 = _shape_prev_y16 + ((pt.y - _shape_prev_y) * zoom) * 128;
			pt.x = ((_shape_cur_x16 + 0x8000) >> 16) + _shape_ix + d;
			pt.y = ((_shape_cur_y16 + 0x8000) >> 16) + _shape_iy + e;
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		//scalePoints(&pt, 1, _vid->_layerScale);
		_gfx.drawPoint(_primitiveColor, &pt);
	} else {
		Point *pt = _vertices;
		int16_t ix, iy;
		_shape_cur_x = ix = READ_BE_UINT16(data) + b; data += 2;
		_shape_cur_y = iy = READ_BE_UINT16(data) + c; data += 2;
		if (_shape_count == 0) {
			f -= ((((_shape_ix - _shape_ox) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((_shape_iy - _shape_oy) * zoom) * 128) + 0x8000) >> 16;
			pt->x = f + _shape_ix + d;
			pt->y = g + _shape_iy + e;
			++pt;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
		} else {
			_shape_cur_x16 = _shape_prev_x16 + ((_shape_cur_x - _shape_prev_x) * zoom) * 128;
			_shape_cur_y16 = _shape_prev_y16 + ((_shape_cur_y - _shape_prev_y) * zoom) * 128;
			pt->x = ix = ((_shape_cur_x16 + 0x8000) >> 16) + _shape_ix + d;
			pt->y = iy = ((_shape_cur_y16 + 0x8000) >> 16) + _shape_iy + e;
			++pt;
		}
		int16_t n = numVertices - 1;
		++numVertices;
		int16_t sx = 0;
		for (; n >= 0; --n) {
			ix = (int8_t)(*data++) + sx;
			iy = (int8_t)(*data++);
			if (iy == 0 && n != 0 && *(data + 1) == 0) {
				sx = ix;
				--numVertices;
			} else {
				sx = 0;
				_shape_cur_x += ix;
				_shape_cur_y += iy;
				_shape_cur_x16 += ix * zoom * 128;
				_shape_cur_y16 += iy * zoom * 128;
				pt->x = ((_shape_cur_x16 + 0x8000) >> 16) + _shape_ix + d;
				pt->y = ((_shape_cur_y16 + 0x8000) >> 16) + _shape_iy + e;
				++pt;
			}
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		//scalePoints(_vertices, numVertices, _vid->_layerScale);
		_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices);
	}
}

void Cutscene::op_drawShapeScale() {
//	debug(DBG_CUT, "Cutscene::op_drawShapeScale()");

	_shape_count = 0;

	int16_t x = 0;
	int16_t y = 0;
	uint16_t shapeOffset = fetchNextCmdWord();
	if (shapeOffset & 0x8000) {
		x = fetchNextCmdWord();
		y = fetchNextCmdWord();
	}

	uint16_t zoom = fetchNextCmdWord() + 512;
	_shape_ix = fetchNextCmdByte();
	_shape_iy = fetchNextCmdByte();

	const uint8_t *shapeOffsetTable    = _polPtr + READ_BE_UINT16(_polPtr + 0x02);
	const uint8_t *shapeDataTable      = _polPtr + READ_BE_UINT16(_polPtr + 0x0E);
	const uint8_t *verticesOffsetTable = _polPtr + READ_BE_UINT16(_polPtr + 0x0A);
	const uint8_t *verticesDataTable   = _polPtr + READ_BE_UINT16(_polPtr + 0x12);

	const uint8_t *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16_t primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	if (primitiveCount != 0) {
		uint16_t verticesOffset = READ_BE_UINT16(shapeData);
		int16_t dx = 0;
		int16_t dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData + 2);
			dy = READ_BE_UINT16(shapeData + 4);
		}
		const uint8_t *p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2) + 1;
		_shape_ox = READ_BE_UINT16(p) + dx; p += 2;
		_shape_oy = READ_BE_UINT16(p) + dy; p += 2;
		while (primitiveCount--) {
			verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
			p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
			dx = 0;
			dy = 0;
			if (verticesOffset & 0x8000) {
				dx = READ_BE_UINT16(shapeData); shapeData += 2;
				dy = READ_BE_UINT16(shapeData); shapeData += 2;
			}
			_hasAlphaColor = (verticesOffset & 0x4000) != 0;
			uint8_t color = *shapeData++;
			/*if (_clearScreen == 0) {
				color += 0x10; // 2nd palette buffer
			}*/
			_primitiveColor = color;// & 0x0f;
			drawShapeScale(p, zoom, dx, dy, x, y, 0, 0);
			++_shape_count;
		}
	}
}

void Cutscene::drawShapeScaleRotate(const uint8_t *data, int16_t zoom, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g) {
//	//emu_printf("Cutscene::drawShapeScaleRotate(%d, %d, %d, %d, %d, %d, %d)\n", zoom, b, c, d, e, f, g);
	_gfx.setLayer(_backPage, _vid->_w);
	uint8_t numVertices = *data++;
	if (numVertices & 0x80) {
		int16_t x, y, ix, iy;
		Point pr[2];
		Point *pt = _vertices;
		_shape_cur_x = ix = b + READ_BE_UINT16(data); data += 2;
		_shape_cur_y = iy = c + READ_BE_UINT16(data); data += 2;
		x = READ_BE_UINT16(data); data += 2;
		y = READ_BE_UINT16(data); data += 2;
		_shape_cur_x16 = _shape_ix - ix;
		_shape_cur_y16 = _shape_iy - iy;
		_shape_ox = _shape_cur_x = _shape_ix + ((_shape_cur_x16 * _rotMat[0] + _shape_cur_y16 * _rotMat[1]) >> 8);
		_shape_oy = _shape_cur_y = _shape_iy + ((_shape_cur_x16 * _rotMat[2] + _shape_cur_y16 * _rotMat[3]) >> 8);
		pr[0].x =  0;
		pr[0].y = -y;
		pr[1].x = -x;
		pr[1].y =  y;
		if (_shape_count == 0) {
			f -= ((_shape_ix - _shape_cur_x) * zoom * 128 + 0x8000) >> 16;
			g -= ((_shape_iy - _shape_cur_y) * zoom * 128 + 0x8000) >> 16;
			pt->x = f;
			pt->y = g;
			++pt;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
		} else {
			_shape_cur_x16 = _shape_prev_x16 + (_shape_cur_x - _shape_prev_x) * zoom * 128;
			_shape_cur_y16 = _shape_prev_y16 + (_shape_cur_y - _shape_prev_y) * zoom * 128;
			pt->x = (_shape_cur_x16 + 0x8000) >> 16;
			pt->y = (_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		for (int i = 0; i < 2; ++i) {
			_shape_cur_x += pr[i].x;
			_shape_cur_x16 += pr[i].x * zoom * 128;
			pt->x = (_shape_cur_x16 + 0x8000) >> 16;
			_shape_cur_y += pr[i].y;
			_shape_cur_y16 += pr[i].y * zoom * 128;
			pt->y = (_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		Point po;
		po.x = _vertices[0].x + d + _shape_ix;
		po.y = _vertices[0].y + e + _shape_iy;
		int16_t rx = _vertices[0].x - _vertices[2].x;
		int16_t ry = _vertices[0].y - _vertices[1].y;
		//scalePoints(&po, 1, _vid->_layerScale);
		_gfx.drawEllipse(_primitiveColor, _hasAlphaColor, &po, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
		pt.x = b + READ_BE_UINT16(data); data += 2;
		pt.y = c + READ_BE_UINT16(data); data += 2;
		_shape_cur_x16 = _shape_ix - pt.x;
		_shape_cur_y16 = _shape_iy - pt.y;
		_shape_cur_x = _shape_ix + ((_rotMat[0] * _shape_cur_x16 + _rotMat[1] * _shape_cur_y16) >> 8);
		_shape_cur_y = _shape_iy + ((_rotMat[2] * _shape_cur_x16 + _rotMat[3] * _shape_cur_y16) >> 8);
		if (_shape_count != 0) {
			_shape_cur_x16 = _shape_prev_x16 + (_shape_cur_x - _shape_prev_x) * zoom * 128;
			pt.x = ((_shape_cur_x16 + 0x8000) >> 16) + _shape_ix + d;
			_shape_cur_y16 = _shape_prev_y16 + (_shape_cur_y - _shape_prev_y) * zoom * 128;
			pt.y = ((_shape_cur_y16 + 0x8000) >> 16) + _shape_iy + e;
		} else {
			f -= (((_shape_ix - _shape_cur_x) * zoom * 128) + 0x8000) >> 16;
			g -= (((_shape_iy - _shape_cur_y) * zoom * 128) + 0x8000) >> 16;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
			pt.x = f + _shape_ix + d;
			pt.y = g + _shape_iy + e;
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		//scalePoints(&pt, 1, _vid->_layerScale);
		_gfx.drawPoint(_primitiveColor, &pt);
	} else {
		int16_t x, y, a, shape_last_x, shape_last_y;
		Point tempVertices[MAX_VERTICES];
		_shape_cur_x = b + READ_BE_UINT16(data); data += 2;
		x = _shape_cur_x;
		_shape_cur_y = c + READ_BE_UINT16(data); data += 2;
		y = _shape_cur_y;
		_shape_cur_x16 = _shape_ix - x;
		_shape_cur_y16 = _shape_iy - y;

		a = _shape_ix + ((_rotMat[0] * _shape_cur_x16 + _rotMat[1] * _shape_cur_y16) >> 8);
		if (_shape_count == 0) {
			_shape_ox = a;
		}
		_shape_cur_x = shape_last_x = a;
		a = _shape_iy + ((_rotMat[2] * _shape_cur_x16 + _rotMat[3] * _shape_cur_y16) >> 8);
		if (_shape_count == 0) {
			_shape_oy = a;
		}
		_shape_cur_y = shape_last_y = a;

		int16_t ix = x;
		int16_t iy = y;
		Point *pt2 = tempVertices;
		int16_t sx = 0;
		for (int16_t n = numVertices - 1; n >= 0; --n) {
			x = (int8_t)(*data++) + sx;
			y = (int8_t)(*data++);
			if (y == 0 && n != 0 && *(data + 1) == 0) {
				sx = x;
				--numVertices;
			} else {
				ix += x;
				iy += y;
				sx = 0;
				_shape_cur_x16 = _shape_ix - ix;
				_shape_cur_y16 = _shape_iy - iy;
				a = _shape_ix + ((_rotMat[0] * _shape_cur_x16 + _rotMat[1] * _shape_cur_y16) >> 8);
				pt2->x = a - shape_last_x;
				shape_last_x = a;
				a = _shape_iy + ((_rotMat[2] * _shape_cur_x16 + _rotMat[3] * _shape_cur_y16) >> 8);
				pt2->y = a - shape_last_y;
				shape_last_y = a;
				++pt2;
			}
		}
		Point *pt = _vertices;
		if (_shape_count == 0) {
			ix = _shape_ox;
			iy = _shape_oy;
			f -= (((_shape_ix - ix) * zoom * 128) + 0x8000) >> 16;
			g -= (((_shape_iy - iy) * zoom * 128) + 0x8000) >> 16;
			pt->x = f + _shape_ix + d;
			pt->y = g + _shape_iy + e;
			++pt;
			_shape_cur_x16 = f << 16;
			_shape_cur_y16 = g << 16;
		} else {
			_shape_cur_x16 = _shape_prev_x16 + ((_shape_cur_x - _shape_prev_x) * zoom * 128);
			pt->x = _shape_ix + d + ((_shape_cur_x16 + 0x8000) >> 16);
			_shape_cur_y16 = _shape_prev_y16 + ((_shape_cur_y - _shape_prev_y) * zoom * 128);
			pt->y = _shape_iy + e + ((_shape_cur_y16 + 0x8000) >> 16);
			++pt;
		}
		for (int i = 0; i < numVertices; ++i) {
			_shape_cur_x += tempVertices[i].x;
			_shape_cur_x16 += tempVertices[i].x * zoom * 128;
			pt->x = d + _shape_ix + ((_shape_cur_x16 + 0x8000) >> 16);
			_shape_cur_y += tempVertices[i].y;
			_shape_cur_y16 += tempVertices[i].y * zoom * 128;
			pt->y = e + _shape_iy + ((_shape_cur_y16 + 0x8000) >> 16);
			++pt;
		}
		_shape_prev_x = _shape_cur_x;
		_shape_prev_y = _shape_cur_y;
		_shape_prev_x16 = _shape_cur_x16;
		_shape_prev_y16 = _shape_cur_y16;
		//scalePoints(_vertices, numVertices + 1, _vid->_layerScale);
		_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices + 1);
	}
}

void Cutscene::op_drawShapeScaleRotate() {
//	emu_printf("Cutscene::op_drawShapeScaleRotate()\n");

	_shape_count = 0;

	int16_t x = 0;
	int16_t y = 0;
	uint16_t shapeOffset = fetchNextCmdWord();
	if (shapeOffset & 0x8000) {
		x = fetchNextCmdWord();
		y = fetchNextCmdWord();
	}

	uint16_t zoom = 512;
	if (shapeOffset & 0x4000) {
		zoom += fetchNextCmdWord();
	}
	_shape_ix = fetchNextCmdByte();
	_shape_iy = fetchNextCmdByte();

	uint16_t r1, r2, r3;
	r1 = fetchNextCmdWord();
	r2 = 180;
	if (shapeOffset & 0x2000) {
		r2 = fetchNextCmdWord();
	}
	r3 = 90;
	if (shapeOffset & 0x1000) {
		r3 = fetchNextCmdWord();
	}
	setRotationTransform(r1, r2, r3);

	const uint8_t *shapeOffsetTable    = _polPtr + READ_BE_UINT16(_polPtr + 0x02);
	const uint8_t *shapeDataTable      = _polPtr + READ_BE_UINT16(_polPtr + 0x0E);
	const uint8_t *verticesOffsetTable = _polPtr + READ_BE_UINT16(_polPtr + 0x0A);
	const uint8_t *verticesDataTable   = _polPtr + READ_BE_UINT16(_polPtr + 0x12);

	const uint8_t *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16_t primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	while (primitiveCount--) {
		uint16_t verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
		const uint8_t *p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
		int16_t dx = 0;
		int16_t dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData); shapeData += 2;
			dy = READ_BE_UINT16(shapeData); shapeData += 2;
		}
		_hasAlphaColor = (verticesOffset & 0x4000) != 0;
		uint8_t color = *shapeData++;
		/*if (_clearScreen == 0) {
			color += 0x10; // 2nd palette buffer
		}*/
		_primitiveColor = color;// & 0x0f;
		drawShapeScaleRotate(p, zoom, dx, dy, x, y, 0, 0);
		++_shape_count;
	}
}
/*
static const uint16_t memoSetPos[] = {
	2, 0xffca, 0x0010, 2, 0xffcb, 0x000f, 2, 0xffcd, 0x000e, 2, 0xffd0, 0x000d, 2, 0xffd3, 0x000c, 2, 0xffd7, 0x000b,
	2, 0xffd9, 0x000a, 2, 0xffdb, 0x0009, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008,
	2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 4, 0xffe2, 0xfffe, 2, 0xffdd, 0x0008,
	4, 0xffe2, 0xfffe, 2, 0xffdd, 0x0008, 4, 0xffe2, 0xfffe, 2, 0xffdd, 0x0008, 4, 0xffe2, 0xfffe, 2, 0xffdd, 0x0008,
	2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008, 2, 0xffdd, 0x0008,
	2, 0xffdc, 0x0008, 2, 0xffda, 0x0008, 2, 0xffd6, 0x0009, 2, 0xffd2, 0x000b, 2, 0xffce, 0x000e, 2, 0xffc9, 0x0010,
	2, 0xffc7, 0x0012, 2, 0xffc8, 0x0013, 2, 0xffca, 0x0015, 2, 0xffce, 0x0014, 2, 0xffd1, 0x0013, 2, 0xffd4, 0x0012,
	2, 0xffd6, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011,
	2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 4, 0xffdc, 0x0009, 2, 0xffd8, 0x0011, 4, 0xffdc, 0x0009, 2, 0xffd8, 0x0011,
	4, 0xffdc, 0x0009, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011, 2, 0xffd8, 0x0011,
	2, 0xffd8, 0x0011, 2, 0xffd7, 0x0011, 2, 0xffd6, 0x0011, 2, 0xffd3, 0x0011, 2, 0xffcd, 0x0012, 2, 0xffc7, 0x0014,
	2, 0xffc1, 0x0016
};

static bool _drawMemoSetShapes;
static uint32_t _memoSetOffset;

static void readSetPalette(const uint8_t *p, uint16_t offset, uint16_t *palette);

static int findSetPaletteColor(const uint16_t color, const uint16_t *paletteBuffer) {
	int index = -1;
	int currentSum = 0;
	for (int l = 0; l < 32; ++l) {
		if (color == paletteBuffer[l]) {
			return l;
		}
		const int dr = ((color >> 8) & 15) - ((paletteBuffer[l] >> 8) & 15);
		const int dg = ((color >> 4) & 15) - ((paletteBuffer[l] >> 4) & 15);
		const int db =  (color       & 15) -  (paletteBuffer[l]       & 15);
		const int sum = 30 * dr * dr + 59 * dg * dg + 11 * db * db;
		if (index < 0 || sum < currentSum) {
			currentSum = sum;
			index = l;
		}
	}
	return index;
}
*/
void Cutscene::op_copyScreen() {
//	emu_printf("Cutscene::op_copyScreen()\n");
	_creditsSlowText = true;
	if (_textCurBuf == _textBuf) {
		++_creditsTextCounter;
	}
	DMA_ScuMemCopy(_backPage, _frontPage, IMG_SIZE);
	_frameDelay = 10;
/*
	const bool drawMemoShapes = _drawMemoSetShapes && (_paletteNum == 19 || _paletteNum == 23) && (_memoSetOffset + 3) <= sizeof(memoSetPos);
	if (drawMemoShapes) {
		uint16_t paletteBuffer[32];
		for (int i = 0; i < 32; ++i) {
			paletteBuffer[i] = READ_BE_UINT16(_palBuf + i * 2);
		}
		uint16_t tempPalette[16];
		readSetPalette(_memoSetShape2Data, 0x462, tempPalette);
		uint8_t paletteLut[32];
		for (int k = 0; k < 16; ++k) {
			const int index = findSetPaletteColor(tempPalette[k], paletteBuffer);
			paletteLut[k] = 0xC0 + index;
		}

		_gfx.setLayer(_backPage, _vid->_w);
		drawSetShape(_memoSetShape2Data, 0, (int16_t)memoSetPos[_memoSetOffset + 1], (int16_t)memoSetPos[_memoSetOffset + 2], paletteLut);
		_memoSetOffset += 3;
		if (memoSetPos[_memoSetOffset] == 4) {
			drawSetShape(_memoSetShape4Data, 0, (int16_t)memoSetPos[_memoSetOffset + 1], (int16_t)memoSetPos[_memoSetOffset + 2], paletteLut);
			_memoSetOffset += 3;
		}
	}
	*/
	updateScreen(); // vbt : remis permet de revoir "present" au boot
}

void Cutscene::op_drawTextAtPos() {
//	emu_printf("Cutscene::op_drawTextAtPos()\n");
	uint16_t strId = fetchNextCmdWord();
	if (strId != 0xFFFF) {
		int16_t x = (int8_t)fetchNextCmdByte() * 8;
		int16_t y = (int8_t)fetchNextCmdByte() * 8;
		if (!_creditsSequence) {
			const uint8_t *str = _res->getCineString(strId & 0xFFF);
			if (str) {
				hasText = true;
//				sameText = false;
				previousP = NULL;
				const uint8_t color = 0xD0 + (strId >> 0xC);
				drawText(x, y, str, color, _vid->_frontLayer, kTextJustifyCenter);
			}
			// 'voyage' - cutscene script redraws the string to refresh the screen
			if (_id == kCineVoyage && (strId & 0xFFF) == 0x45) {
				if ((_cmdPtr - _cmdPtrBak) == 0xA) {
//					_stub->copyRect(0, 224, 512, 224, _frontPage, _vid->_w);				
				} else {
					_stub->sleep(15);
				}
			}
		}
	}
}

void Cutscene::op_handleKeys() {
//	debug(DBG_CUT, "Cutscene::op_handleKeys()");
	while (1) {
		uint8_t key_mask = fetchNextCmdByte();
		if (key_mask == 0xFF) {
			return;
		}
		bool b = true;
		switch (key_mask) {
		case 1:
			b = (_stub->_pi.dirMask & PlayerInput::DIR_UP) != 0;
			break;
		case 2:
			b = (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) != 0;
			break;
		case 4:
			b = (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) != 0;
			break;
		case 8:
			b = (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) != 0;
			break;
		case 0x80:
			b = _stub->_pi.space || _stub->_pi.enter || _stub->_pi.shift;
			break;
		}
		if (b) {
			break;
		}
		_cmdPtr += 2;
	}
	_stub->_pi.dirMask = 0;
	_stub->_pi.enter = false;
	_stub->_pi.space = false;
	_stub->_pi.shift = false;
	int16_t n = fetchNextCmdWord();
	if (n < 0) {
		n = -n - 1;
		if (_varKey == 0) {
			_stop = true;
			return;
		}
		if (_varKey != n) {
			_cmdPtr = _cmdPtrBak;
			return;
		}
		_varKey = 0;
		--n;
		_cmdPtr = getCommandData();
		n = READ_BE_UINT16(_cmdPtr + 2 + n * 2);
		if (_res->isMac()) {
			const int count = READ_BE_UINT16(_cmdPtr);
//			assert(n < count);
			if(n>=count)
				return;
			_cmdPtr += n;
			_cmdPtrBak = _cmdPtr;
			return;
		}
	}
	_cmdPtr = _cmdPtrBak = getCommandData() + n + _baseOffset;
}

uint8_t Cutscene::fetchNextCmdByte() {
	return *_cmdPtr++;
}

uint16_t Cutscene::fetchNextCmdWord() {
//	//emu_printf("_cmdPtr %p\n",_cmdPtr);
	uint16_t i = READ_BE_UINT16(_cmdPtr);
	_cmdPtr += 2;
	return i;
}

void Cutscene::mainLoop(uint16_t num) {
	_frameDelay = 5;
	_tstamp = _stub->getTimeStamp();

	Color c;
	c.r = c.g = c.b = 0;
	for (int i = 0; i < 0x20; ++i) {
		_stub->setPaletteEntry(0x1C0 + i, &c);
	}
	
	if (_id != 0x4A && !_creditsSequence) {
//		_ply->play(_musicTableDOS[_id],0);
emu_printf("_id %d _musicTableDOS %d\n",_id,_musicTableDOS[_id]);
		_mix.playMusic(_musicTableDOS[_id]);
	}
	if(_id == 0x4A)
	{
		_mix.unpauseMusic();		
	}
	_newPal = false;
	_hasAlphaColor = false;
	const uint8_t *p = getCommandData();
	const int count = READ_BE_UINT16(p);
	int offset = 0;
	_baseOffset = (count + 1) * 2;
	if (_res->isMac() && _id != 3 && _id != 19 && _id != 22 && _id != 23 && _id != 24) {
		_vid->_charShadowColor = 0xE0;
//		assert(num < count);
		if(num >= count)
			return;
		offset = READ_BE_UINT16(p + 2 + num * 2);
//		assert(offset >= _baseOffset);
		if(offset < _baseOffset)
			return;
	} else {
		if (num != 0) {
			offset = READ_BE_UINT16(p + 2 + num * 2);
		} else if (count != 0) {
			const int startOffset = READ_BE_UINT16(p + 2);
			if (startOffset != 0) {
				warning("startOffset %d count %d num %d", startOffset, count, num);
			}
		}
		p += _baseOffset;
	}
#if 0
	for (int i = 0; i < count; ++i) {
		fprintf(stdout, "cutscene start point %d offset 0x%x, base 0x%x\n", i, READ_BE_UINT16(p + 2 + i * 2), (count + 1) * 2);
	}
#endif
	_varKey = 0;
	_cmdPtr = _cmdPtrBak = p + offset;
	_polPtr = getPolygonData();

	_paletteNum = -1;
//	_drawMemoSetShapes = (_id == kCineMemo);
//	_memoSetOffset = 0;
	while (!_stub->_pi.quit && !_interrupted && !_stop) {
		uint8_t op = fetchNextCmdByte();
		if (op & 0x80) {
			break;
		}
		op >>= 2;
		if (op >= NUM_OPCODES) {
//			emu_printf("Invalid cutscene opcode = 0x%02X\n", op);
			continue;
		}
//emu_printf("VBT cutmainLoop k op %d\n",op);
		(this->*_opcodeTable[op])();
		_stub->processEvents();
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			_interrupted = true;
		}
	}
//		//emu_printf("VBT cutmainLoop h\n");
	_stop=true;
}

bool Cutscene::load(uint16_t cutName) {
//emu_printf(" Cutscene::load %d id %d\n", cutName, _id);
//	assert(cutName != 0xFFFF);
	if(cutName == 0xFFFF)
		return 0;
/*	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();
*/	slTVOff();
	slSynch();
	cutName &= 0xFF;
	const char *name = _namesTableDOS[cutName];
	if(cutName!=12 && cutName!=31 && cutName!=2)
	{
		_res->MAC_loadCutscene(name);
	}
	else
	{
		_res->MAC_closeMainFile();
		_res->load(name, Resource::OT_CMP);
		_res->MAC_reopenMainFile();
	}
	slTVOn();	
/*	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration MAC_loadCutscene : %d\n",e-s);
*/
/*		break;
	}*/
//	e = _stub->getTimeStamp();
	bool loaded = (_res->_cmd && _res->_pol);
////emu_printf(" Cutscene::end load %x %d\n", cutName,(_res->_cmd && _res->_pol));

	if(!loaded)
		unload();
	return loaded;
}

void Cutscene::unload() {
/*	switch (_res->_type) {
	case kResourceTypeDOS:
		_res->unload(Resource::OT_CMD);
		_res->unload(Resource::OT_POL);
		break;
	case kResourceTypeMac:*/
		if (_id != 0x3D)// && _id != 40 && _id != 69)
		_res->MAC_unloadCutscene();
/*		break;
	}
*/	
	if (/*_res->isMac() &&*/ _id != 0x48 && _id != 0x49)
	{
		_vid->SAT_cleanSprites();
#ifndef SUBTITLE_SPRITE
//			emu_printf("clean front1\n");
		memset4_fast(&_vid->_frontLayer[96*_vid->_w],0x00,_vid->_w* (_vid->_h-128));
		_stub->copyRect(0, 96, _vid->_w, _vid->_h-128, _vid->_frontLayer, _vid->_w);
#endif
//		Color clut[512];
		Color *clut = (Color *)hwram_screen;
		_res->MAC_copyClut16(clut, 0x1C, 0x37);  // icons
		_res->MAC_copyClut16(clut, 0x1D, 0x38);

		const int baseColor = 12 * 16 + 256;
		for (int i = 0; i < 32; ++i) {
			int color = baseColor + i;
			_stub->setPaletteEntry(color, &clut[color]);
		}
	}
	slTVOn();
}

void Cutscene::prepare() {
//	_frontPage = (uint8_t *)_res->_scratchBuffer;
//	_backPage = (uint8_t *)_res->_scratchBuffer+(IMG_SIZE*1);
//	_auxPage = (uint8_t *)_res->_scratchBuffer+(IMG_SIZE*2);
	_frontPage = (uint8_t *)hwram_screen;
	_backPage = (uint8_t *)_res->_scratchBuffer;
	_auxPage = (uint8_t *)hwram_screen+IMG_SIZE;
slTVOff();
slSynch();
	memset4_fast(&_vid->_frontLayer[52*_vid->_w], 0x00,16*_vid->_w);
	_stub->copyRect(0, 52, 480, 16, _vid->_frontLayer, _vid->_w);
//emu_printf("prepare cutscene\n");	
	memset4_fast(_auxPage, 0x00, IMG_SIZE/2);
	memset4_fast(_backPage, 0x00, IMG_SIZE);
	memset4_fast(_frontPage, 0x00, IMG_SIZE);
	memset4_fast((uint8_t *)(SpriteVRAM + 0x80000 - IMG_SIZE*2), 0x00, IMG_SIZE*2);
#ifdef SUBTITLE_SPRITE	
	memset((uint8_t *)_vid->_txt2Layer,0, 480*255);
#endif
	_stub->_pi.dirMask = 0;
	_stub->_pi.enter = false;
	_stub->_pi.space = false;
	_stub->_pi.shift = false;
	_interrupted = false;
	_stop = false;
	hasText = false;
	const int w = 240;
	const int h = 128;
	const int x = 0;//(Video::GAMESCREEN_W - w) / 2;
	const int y = 0;//50;
	const int sw = w;// * _vid->_layerScale;
	const int sh = h;// * _vid->_layerScale;
	const int sx = x; //* _vid->_layerScale;
	const int sy = y;// * _vid->_layerScale;
	_gfx.setClippingRect(sx, sy, sw, sh);
	slScrAutoDisp(NBG1ON|SPRON); // vbt à remettre
	slTVOn();
}

void Cutscene::playCredits() {
//	emu_printf("Cutscene::playCredits()\n");
//	if (_res->isMac()) {
		_res->MAC_loadCreditsText();
		_creditsTextIndex = 0;
		_creditsTextLen = 0;
//	} else {
//		_textCurPtr = _creditsDataDOS;
//	}
	_textBuf[0] = 0xA;
	_textCurBuf = _textBuf;
	_creditsSequence = true;
	_creditsSlowText = false;
	_creditsKeepText = false;
	_creditsTextCounter = 0;
	_interrupted = false;
	const uint8_t *cut_seq = _creditsCutSeq;
	while (!_stub->_pi.quit && !_interrupted) {
		const uint8_t cut_id = *cut_seq++;
		if (cut_id == 0xFF) {
			break;
		}
		prepare();
		const uint16_t *offsets = _offsetsTableDOS;
		uint16_t cutName = offsets[cut_id * 2 + 0];
		uint16_t cutOff  = offsets[cut_id * 2 + 1];
		if (load(cutName)) {
			mainLoop(cutOff);
			unload();
		}
	}
	_creditsSequence = false;
}

void Cutscene::play() {
	if (_id != 0xFFFF) {
		_textCurBuf = NULL;
//		emu_printf("Cutscene::play() _id=0x%X c%p s %p\n", _id , current_lwram, save_current_lwram);
		_creditsSequence = false;
		prepare();
		const uint16_t *offsets = _offsetsTableDOS;

		uint16_t cutName = offsets[_id * 2 + 0];
		uint16_t cutOff  = offsets[_id * 2 + 1];
		if (cutName == 0xFFFF) {
			switch (_id) {
			case 3: // keys
				//if (g_options.play_carte_cutscene) 
				{
					cutName = 2; // CARTE
				}
				break;
			case 8: // save checkpoints
				_res->MAC_closeMainFile();
				GFS_Load(GFS_NameToId((int8_t *)"CAILLOU.CMP"),0,(void *)current_lwram,6361);
				playSet(current_lwram, 0x5E4);
				_res->MAC_reopenMainFile();
				break;
			case 19:
				//if (g_options.play_serrure_cutscene) 
				{
					cutName = 31; // SERRURE
				}
				break;
			case 22: // Level 2 fuse repaired
			case 23: // switches
			case 24: // Level 2 fuse is blown
				//if (g_options.play_asc_cutscene) 
				{
					cutName = 12; // ASC
				}
				break;
/*			case 30:
			case 31:
				//if (g_options.play_metro_cutscene) 
				{
					cutName = 14; // METRO
				}*/
				break;
			case 46: // Level 2 terminal card mission
				break;
			default:
//				emu_printf("Unknown cutscene %d\n", _id);
				break;
			}
		}
/*
		if (_patchedOffsetsTable) {
			for (int i = 0; _patchedOffsetsTable[i] != 255; i += 3) {
				if (_patchedOffsetsTable[i] == _id) {
					cutName = _patchedOffsetsTable[i + 1];
					cutOff = _patchedOffsetsTable[i + 2];
					break;
				}
			}
		}
		if (g_options.use_text_cutscenes) 
		{
			const Text *textsTable = (_res->_lang == LANG_FR) ? _frTextsTable : _enTextsTable;
			for (int i = 0; textsTable[i].str; ++i) {
				if (_id == textsTable[i].num) {
					playText(textsTable[i].str);
					break;
				}
			}
		} else*/
		if (cutName != 0xFFFF) {
			if (load(cutName)) {
				mainLoop(cutOff);
				unload();
			}
		}
		if (_id != 0x3D) {
			_id = 0xFFFF;
		}
	}
}

static void readSetPalette(const uint8_t *p, uint16_t offset, uint16_t *palette) {
	offset += 12;
	for (int i = 0; i < 16; ++i) {
		const uint16_t color = READ_BE_UINT16(p + offset); offset += 2;
		palette[i] = color;
	}
}

void Cutscene::drawSetShape(const uint8_t *p, uint16_t offset, int x, int y, const uint8_t *paletteLut) {
//emu_printf("Cutscene::drawSetShape(%p,%d,%d,%d,%p)\n",p,offset,x,y,paletteLut);	
	const int count = READ_BE_UINT16(p + offset); offset += 2;
	for (int i = 0; i < count - 1; ++i) {
		offset += 5; // shape_marker
		const int verticesCount = p[offset++];
		const int ix = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
		const int iy = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
		uint8_t color = paletteLut[p[offset]]; offset += 2;

		if (verticesCount == 255) {
			int16_t rx = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
			int16_t ry = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
			Point pt;
			pt.x = x + ix;
			pt.y = y + iy;
			//scalePoints(&pt, 1, _vid->_layerScale);
			_gfx.drawEllipse(color, false, &pt, rx, ry);
		} else {
			for (int i = 0; i < verticesCount; ++i) {
				_vertices[i].x = x + (int16_t)READ_BE_UINT16(p + offset); offset += 2;
				_vertices[i].y = y + (int16_t)READ_BE_UINT16(p + offset); offset += 2;
			}
			//scalePoints(_vertices, verticesCount, _vid->_layerScale);
			_gfx.drawPolygon(color, false, _vertices, verticesCount);
		}
	}
}

static uint16_t readSetShapeOffset(const uint8_t *p, int offset) {
	const int count = READ_BE_UINT16(p + offset); offset += 2;
	for (int i = 0; i < count - 1; ++i) {
		offset += 5; // shape_marker
		const int verticesCount = p[offset++];
		offset += 6;
		if (verticesCount == 255) {
			offset += 4; // ellipse
		} else {
			offset += verticesCount * 4; // polygon
		}
	}
	return offset;
}

static const int kMaxShapesCount = 16;
static const int kMaxPaletteSize = 32;

void Cutscene::playSet(const uint8_t *p, int offset) {
	SetShape backgroundShapes[kMaxShapesCount];
	const int bgCount = READ_BE_UINT16(p + offset); offset += 2;
	assert(bgCount <= kMaxShapesCount);
	for (int i = 0; i < bgCount; ++i) {
		uint16_t nextOffset = readSetShapeOffset(p, offset);
		backgroundShapes[i].offset = offset;
		backgroundShapes[i].size = nextOffset - offset;
		offset = nextOffset + 45;
	}
	SetShape foregroundShapes[kMaxShapesCount];
	const int fgCount = READ_BE_UINT16(p + offset); offset += 2;
	assert(fgCount <= kMaxShapesCount);
	for (int i = 0; i < fgCount; ++i) {
		uint16_t nextOffset = readSetShapeOffset(p, offset);
		foregroundShapes[i].offset = offset;
		foregroundShapes[i].size = nextOffset - offset;
		offset = nextOffset + 45;
	}

	prepare();
	_gfx.setLayer(_backPage, _vid->_w);

	offset = 10;
	const int frames = READ_BE_UINT16(p + offset); offset += 2;
	for (int i = 0; i < frames && !_stub->_pi.quit && !_interrupted; ++i) {
		const uint32_t timestamp = _stub->getTimeStamp();

		memset(_backPage, 0xC0, _vid->_h * _vid->_w);

		const int shapeBg = READ_BE_UINT16(p + offset); offset += 2;
		const int count = READ_BE_UINT16(p + offset); offset += 2;

		uint16_t paletteBuffer[kMaxPaletteSize];
		memset(paletteBuffer, 0, sizeof(paletteBuffer));
		readSetPalette(p, backgroundShapes[shapeBg].offset + backgroundShapes[shapeBg].size, paletteBuffer);
		int paletteLutSize = 16;

		uint8_t paletteLut[kMaxPaletteSize];
		for (int j = 0; j < 16; ++j) {
			paletteLut[j] = 0xC0 + j;
		}

		drawSetShape(p, backgroundShapes[shapeBg].offset, 0, 0, paletteLut);
		for (int j = 0; j < count; ++j) {
			const int shapeFg = READ_BE_UINT16(p + offset); offset += 2;
			const int shapeX = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
			const int shapeY = (int16_t)READ_BE_UINT16(p + offset); offset += 2;

			uint16_t tempPalette[16];
			readSetPalette(p, foregroundShapes[shapeFg].offset + foregroundShapes[shapeFg].size, tempPalette);
			for (int k = 0; k < 16; ++k) {
				bool found = false;
				for (int l = 0; l < paletteLutSize; ++l) {
					if (tempPalette[k] == paletteBuffer[l]) {
						found = true;
						paletteLut[k] = 0xC0 + l;
						break;
					}
				}
				if (!found) {
//					assert(paletteLutSize < kMaxPaletteSize);
					if(paletteLutSize >= kMaxPaletteSize)
						return;
					paletteLut[k] = 0xC0 + paletteLutSize;
					paletteBuffer[paletteLutSize++] = tempPalette[k];
				}
			}

			drawSetShape(p, foregroundShapes[shapeFg].offset, shapeX, shapeY, paletteLut);
		}

		for (int j = 0; j < paletteLutSize; ++j) {
//			Color c = Video::AMIGA_convertColor(paletteBuffer[j]);
			
			Color c;
			uint8 t = (paletteBuffer[j] == 0) ? 0 : 3;			
// vbt correction des couleurs			
			c.r = ((paletteBuffer[j] & 0xF00) >> 6) | t;
			c.g = ((paletteBuffer[j] & 0x0F0) >> 2) | t;
			c.b = ((paletteBuffer[j] & 0x00F) << 2) | t;
			
			_stub->setPaletteEntry(0x1C0 + j, &c);
		}
/*
		_stub->copyRect(0, 0, _vid->_w, _vid->_h, _backPage, _vid->_w);
		_stub->updateScreen(0);
*/

		_vid->SAT_displayCutscene(0,0, 0, 128, 240);
		slSynch();
		updatePalette();
		const int diff = 90 - (_stub->getTimeStamp() - timestamp);
		_stub->sleep((diff < 16) ? 16 : diff);
		_stub->processEvents();
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			_interrupted = true;
		}
	}
	_stop = true; // pour reprendre la musique
}
