//#pragma GCC optimize ("O2")
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
#include "sat_mem_checker.h"
extern Uint8 *hwram_screen;
extern Uint8 *hwram_ptr;
extern Uint8 *current_lwram;
//extern Uint8 *save_current_lwram;
extern Uint8 frame_x;
extern Uint8 frame_y;
extern Uint8 frame_z;
void *memset4_fast(void *, long, size_t);
Uint8 transferAux=0;
extern Uint32 gfsLibWork[GFS_WORK_SIZE(OPEN_MAX)/sizeof(Uint32)];
int16_t previousStr=-1;
}
#include "mod_player.h"
#include "decode_mac.h"
#include "resource.h"
#include "systemstub.h"
#include "cutscene.h"
#include "saturn_print.h"
#include "video.h"

extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
//extern volatile Uint8 audioEnabled;
#define CLEAN_X 16
#define CLEAN_Y 114
#define CLEAN_W 480
#define CLEAN_H 224
#define CLEAN_H_MORE 304
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
	_isConcavePolygonShape = false;
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
	int32_t delay = _stub->getTimeStamp() - _tstamp;

//	emu_printf("sleepx frame delay %d delay %d _tstamp %d\n", frameDelay, delay, _tstamp);
	delay=(delay<0?17:delay);
	const int32_t pause = frameDelay * (1000 / frameHz) - delay;
#if 1
	const int32_t target = frameDelay * (1000 / frameHz);
	if (pause<-4)
		emu_printf("too slow !! target: %d ms, actual: %d ms, overtime: %d ms\n",
		target, delay, delay - target);
#endif
	if (pause > 0) {
		_stub->sleep(pause);
	}
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

void Cutscene::updateScreen() {
#ifdef DEBUG_CUTSCENE
DecodeBuffer buf{};
#endif
//emu_printf("VBT updateScreen1 duration %d\n",_stub->getTimeStamp() - _tstamp);

	if(hasText)
	{
		_stub->copyRect(CLEAN_X, CLEAN_Y, CLEAN_W, CLEAN_H_MORE, _vid->_frontLayer, _vid->_w);
//		memset4_fast(&_vid->_frontLayer[96*_vid->_w],0x0000,_vid->_w* 320); // nettoyeur de texte du bas
		hasText = false;
	}
//emu_printf("VBT updateScreen2 duration %d\n",_stub->getTimeStamp() - _tstamp);
	sync(_frameDelay - 1);
	SWAP(_frontPage, _backPage);


    SPRITE user_sprite;
    user_sprite.PMOD = CL16Bnk | ECdis | 0x0800;
    user_sprite.COLR = 0xC0;
    user_sprite.SIZE = 0x1E80;
    user_sprite.CTRL = FUNC_Sprite | _ZmCC;
    user_sprite.XA = 0;
    user_sprite.YA = 0;
    user_sprite.XB = 0 + 479;//(239 << 1);
    user_sprite.YB = 0 + 255;//(127 << 1);
    user_sprite.GRDA = 0;
    size_t spriteVramOffset;
	const Uint16 size = (IMG_SIZE >> 1);
	
    if (transferAux) {
		spriteVramOffset = 0x80000 - size;
        slTransferEntry((void*)_auxPage, (void*)(SpriteVRAM + spriteVramOffset), size);
    }
    user_sprite.SRCA = spriteVramOffset / 8;
    slSetSprite(&user_sprite, toFIXED2(240));

    user_sprite.COLR = 0xD0;
//    spriteVramOffset = 0x80000 - IMG_SIZE - ((_frontPage == hwram_screen+IMG_SIZE) ? (IMG_SIZE >> 1) : 0);
    spriteVramOffset = 0x80000 - IMG_SIZE - ((_frontPage == hwram_screen) ? 0 : size);
    user_sprite.SRCA = spriteVramOffset / 8;

    uint8_t *aux = (uint8_t *)(SpriteVRAM + spriteVramOffset);  // Use pointers to avoid array indexing overhead

	packPixels(_frontPage, hwram_ptr, IMG_SIZE);
	DMA_ScuMemCopy(aux, hwram_ptr, size);

    slSetSprite(&user_sprite, toFIXED2(240));
    frame_x++;

	if(_id == 0x48)
	{
		_vid->SAT_displayMeshSprite(-229, 239, -82, -36);
		_vid->SAT_displayMeshSprite(-153, 239, -20, 10);
		_vid->SAT_displayMeshSprite(-141, 125, 26, 46);
	}
    slSynch();
//	_stub->initTimeStamp();
    updatePalette();
    transferAux = 0;

#ifdef DEBUG_CUTSCENE
    #define SAT_ALIGN(a) ((a+3)&~3)
    buf.dst_w = 240;
    buf.dst_h = 128;
    position_vram = 0x1000;
    size_t dataSize = SAT_ALIGN(buf.dst_w * buf.dst_h);
    int cgaddr1 = 8 * _vid->SAT_copySpriteToVram((uint8_t *)_backPage, buf, dataSize);

    // Debug loop for _backPage
    uint8_t *aux2 = (uint8_t *)(SpriteVRAM + cgaddr1);
    packPixels(_backPage, aux2, IMG_SIZE);
 //   slTransferEntry(temp, aux2, IMG_SIZE / 2);
    _vid->SAT_displaySprite((uint8_t *)cgaddr1, -320, -224, 128, 240);

    int cgaddr3 = _vid->SAT_copySpriteToVram((uint8_t *)_frontPage, buf, dataSize);

    // Debug loop for _frontPage
    uint8_t *aux1 = (uint8_t *)(SpriteVRAM + cgaddr3 * 8);
    packPixels(_frontPage, aux1, IMG_SIZE);
//    slTransferEntry(temp, aux1, IMG_SIZE / 2);
    _vid->SAT_displaySprite((uint8_t *)(cgaddr3 * 8), -320, -96, 128, 240);

    int cgaddr2 = _vid->SAT_copySpriteToVram((uint8_t *)_auxPage, buf, dataSize);
    _vid->SAT_displaySprite((uint8_t *)(cgaddr2 * 8), -320, 32, 128, 240);
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
#if 0
static uint8_t getNewLineChar(const Resource *res) {
	return /*(res->_lang == LANG_JP) ? 0xD1 :*/ '|';
}
#endif
uint16_t Cutscene::findTextSeparators(const uint8_t *p, int len) {
	const uint8_t newLineChar = '|';//getNewLineChar(_res);
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

#if 1
void Cutscene::drawText(int16_t x, int16_t y, const uint8_t *p, uint16_t color, uint8_t *page, int textJustify) {
//emu_printf("Cutscene::drawText(x=%d, y=%d, c=%d, justify=%d %p len %d %p)\n", x, y, color, textJustify,p,p[1],previousP);
//	memset(&_vid->_frontLayer[y*_vid->_w],0x00,512*(440-y)); // vbt : à voir
	int len = 0;
//	int sameP = (previousP == p);
//	previousP = (uint8_t *)p;
	if (p != _textBuf /*&& _res->isMac()*/) {
		len = *p++;
	} else {
		while (p[len] != 0xA && p[len]) {
			++len;
		}
	}
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
	const uint8_t newLineChar = '|';//getNewLineChar(_res);
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
#endif
#if 0
void Cutscene::drawText(int16_t x, int16_t y, const uint8_t *p, uint16_t color, uint8_t *page, int textJustify) {
    // Early exit optimization - check text similarity first
    int len = 0;
    int sameP = (previousP == p);
    previousP = (uint8_t *)p;
    
    if (p != _textBuf) {
        len = *p++;
    } else {
        // Optimized length calculation using word operations where possible
        const uint32_t *p32 = (const uint32_t *)p;
        const uint32_t *end32 = p32;
        
        // Check 4 bytes at a time for 0x0A or 0x00
        while (true) {
            uint32_t chunk = *end32;
            if ((chunk & 0xFF) == 0x0A || (chunk & 0xFF) == 0x00) break;
            if (((chunk >> 8) & 0xFF) == 0x0A || ((chunk >> 8) & 0xFF) == 0x00) { len += 1; break; }
            if (((chunk >> 16) & 0xFF) == 0x0A || ((chunk >> 16) & 0xFF) == 0x00) { len += 2; break; }
            if (((chunk >> 24) & 0xFF) == 0x0A || ((chunk >> 24) & 0xFF) == 0x00) { len += 3; break; }
            len += 4;
            end32++;
        }
    }
    
    const uint8_t *fnt = _res->_fnt;
    uint16_t lastSep = 0;
    
    if (textJustify != kTextJustifyLeft) {
        lastSep = findTextSeparators(p, len);
        if (textJustify != kTextJustifyCenter) {
            lastSep = 30;
        }
    }
    
    const uint8_t *sep = _textSep;
    y += 50;
    x += 8;
    int16_t yPos = y;
    int16_t xPos = x;
    
    if (textJustify != kTextJustifyLeft) {
        xPos += ((lastSep - *sep++) / 2) * Video::CHAR_W;
    }
    
    const uint8_t newLineChar = getNewLineChar(_res);
    
    // Main rendering loop with optimized character handling
    for (int i = 0; i < len && p[i] != 0xA; ++i) {
        uint8_t ch = p[i];
        
        if (ch == newLineChar) {
            yPos += Video::CHAR_H;
            xPos = x;
            if (textJustify != kTextJustifyLeft) {
                xPos += ((lastSep - *sep++) / 2) * Video::CHAR_W;
            }
        } else if (ch == 0x20) {
            xPos += Video::CHAR_W;
        } else if (ch != 0x9) {  // Skip tabs
            _vid->MAC_drawStringChar(page, _vid->_w, xPos, yPos, fnt, color, ch);
            xPos += Video::CHAR_W;
        }
    }
}
#endif
inline void Cutscene::clearBackPage() {
//	emu_printf("clearBackPage : _clearScreen %d\n",_clearScreen);
	memset4_fast(_backPage, 0, IMG_SIZE);
}

void Cutscene::drawCreditsText() {
//	_stub->initTimeStamp();
	if (_creditsKeepText) {
		if (_creditsSlowText) {
			return;
		}
		_creditsKeepText = false;
//		emu_printf("drawCreditsText on nettoie1 ?\n");
	}
	if (_creditsTextCounter <= 0) {
		uint8_t code;
//		const bool isMac = _res->isMac();
		if (/*isMac &&*/ _creditsTextLen <= 0) {
			memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9);

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
		if (code == 0x7D || code == 0x5E/*&& isMac*/) {
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
//	unsigned int s = _stub->getTimeStamp();	
	drawText((_creditsTextPosX - 1) * 8, _creditsTextPosY * 8, _textBuf, 0xEF, _vid->_frontLayer, kTextJustifyLeft);
//	unsigned int e = _stub->getTimeStamp();
//	if(e-s!=0)
//	emu_printf("--duration drawCreditsText : %d\n",e-s);	
}

void Cutscene::op_markCurPos() {
//	emu_printf("Cutscene::op_markCurPos()\n");
	_cmdStartPtr = _cmdPtr;
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
//	emu_printf("op_refreshScreen Cutscene::clearBackPage()\n");
		clearBackPage();
		_creditsSlowText = false;
	}
}

void Cutscene::op_waitForSync() {
//	emu_printf("Cutscene::op_waitForSync()\n");
//emu_printf("VBT op_waitForSync duration %d\n",_stub->getTimeStamp() - _tstamp);
	
	if (_creditsSequence) {
		uint16_t n = fetchNextCmdByte() * 2;
		do {
			_creditsSlowText = true;
			_frameDelay = 3;
			if (_textBuf == _textCurBuf) {
				_creditsTextCounter = /*_res->isDOS() ? 20 :*/ 60;
			}
//emu_printf("VBT drawCreditsText1 duration %d\n",_stub->getTimeStamp() - _tstamp);
			drawCreditsText();
//emu_printf("VBT drawCreditsText2 duration %d\n",_stub->getTimeStamp() - _tstamp);
//			updateScreen();
		} while (--n);
//		clearBackPage();
		_creditsSlowText = false;
	} else {
		_frameDelay = fetchNextCmdByte() * 4;
//emu_printf("VBT sync %d duration %d\n",_frameDelay, _stub->getTimeStamp() - _tstamp);
		sync(_frameDelay);
/*		if(_id != 17) // VBt : à ne pas remettre
		{
			_stub->copyRect(CLEAN_X, CLEAN_Y, CLEAN_W, CLEAN_H_MORE, _vid->_frontLayer, _vid->_w);
		}*/
	}
//	emu_printf("VBT op_waitForSyncEnd duration %d\n",_stub->getTimeStamp() - _tstamp);
}

void Cutscene::checkShape(uint16_t shapeOffset) {
	_isConcavePolygonShape = /*_res->isMac() &&*/ _id == kCineLogos && (shapeOffset & 0x7FF) == 2;
}

void Cutscene::drawShape(const uint8_t *data, int16_t x, int16_t y) {
//emu_printf("Cutscene::drawShape()1\n");
	_gfx.setLayer(_backPage);
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
		if (_isConcavePolygonShape) {
			_gfx.floodFill(_primitiveColor, _vertices, numVertices);
		} else {
			_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices);
		}
	}
}

void Cutscene::packPixels(uint8_t *back, uint8_t *aux, size_t size) {
/*
   uint32_t *b = (uint32_t *)back;
    for (size_t i = 0, j = 0; i < size / 4; i += 4, j += 8) {
        uint32_t b0 = b[i];
        uint32_t b1 = b[i + 1];
        uint32_t b2 = b[i + 2];
        uint32_t b3 = b[i + 3];

        aux[j]     = ((b0 >> 16) & 0xFF) | ((b0 >> 24) << 4);
        aux[j + 1] = ((b0 >> 0)  & 0xFF) | ((b0 >> 8)  << 4);
        aux[j + 2] = ((b1 >> 16) & 0xFF) | ((b1 >> 24) << 4);
        aux[j + 3] = ((b1 >> 0)  & 0xFF) | ((b1 >> 8)  << 4);
        aux[j + 4] = ((b2 >> 16) & 0xFF) | ((b2 >> 24) << 4);
        aux[j + 5] = ((b2 >> 0)  & 0xFF) | ((b2 >> 8)  << 4);
        aux[j + 6] = ((b3 >> 16) & 0xFF) | ((b3 >> 24) << 4);
        aux[j + 7] = ((b3 >> 0)  & 0xFF) | ((b3 >> 8)  << 4);
    }
*/

    constexpr int CHUNK_SIZE = 3840;
    constexpr int PIXELS_PER_CHUNK = CHUNK_SIZE * 2;
    constexpr int TOTAL_PIXELS = 240 * 128;
    constexpr int TOTAL_CHUNKS = TOTAL_PIXELS / PIXELS_PER_CHUNK;

    uint8_t *buffer = (uint8_t*)hwram_screen+(HWRAM_SCREEN_SIZE-CHUNK_SIZE);
    const uint8_t* src = back;
    size_t spriteVramOffset = 0;

    for (int chunk = 0; chunk < TOTAL_CHUNKS; ++chunk) {
        for (int i = 0; i < PIXELS_PER_CHUNK; i+=2) {
			uint8_t hi = src[i];
			uint8_t lo = src[i + 1];
			buffer[i>>1] = (hi << 4) | lo;
        }
//		DMA_ScuMemCopy((void*)(aux + spriteVramOffset), (void*)temp240buffer, CHUNK_SIZE); // ne fonctionne pas
//      slTransferEntry((void*)temp240buffer, (void*)(aux + spriteVramOffset), CHUNK_SIZE);
		memcpyl((void*)(aux + spriteVramOffset), (void*)buffer, CHUNK_SIZE);

        spriteVramOffset += CHUNK_SIZE;
        src += PIXELS_PER_CHUNK;
    }
}

void Cutscene::op_drawShape() {
//emu_printf("op_drawShape\n");
    int16_t x = 0, y = 0;
    uint16_t shapeOffset = fetchNextCmdWord();
    if (shapeOffset & 0x8000) {
        x = fetchNextCmdWord();
        y = fetchNextCmdWord();
    }
    checkShape(shapeOffset);

    const uint8_t *shapeOffsetTable = _polPtr + READ_BE_UINT16(_polPtr + 0x02);
    const uint8_t *shapeDataTable = _polPtr + READ_BE_UINT16(_polPtr + 0x0E);
    const uint8_t *verticesOffsetTable = _polPtr + READ_BE_UINT16(_polPtr + 0x0A);
    const uint8_t *verticesDataTable = _polPtr + READ_BE_UINT16(_polPtr + 0x12);

    const uint8_t *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
    uint16_t primitiveCount = READ_BE_UINT16(shapeData);
    shapeData += 2;

    while (primitiveCount--) {
        uint16_t verticesOffset = READ_BE_UINT16(shapeData);
        shapeData += 2;
        const uint8_t *primitiveVertices = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
        int16_t dx = 0, dy = 0;
        if (verticesOffset & 0x8000) {
            dx = READ_BE_UINT16(shapeData);
            shapeData += 2;
            dy = READ_BE_UINT16(shapeData);
            shapeData += 2;
        }
        _hasAlphaColor = (verticesOffset & 0x4000) != 0;
        _primitiveColor = *shapeData++;
        drawShape(primitiveVertices, x + dx, y + dy);
    }

    if (_clearScreen != 0) {
        packPixels(_backPage, _auxPage, IMG_SIZE);
        clearBackPage();
        transferAux = 1;
    }
}

//static int _paletteNum = -1;

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
//	_paletteNum = num;
}

void Cutscene::op_drawCaptionText() {
	uint16_t strId = fetchNextCmdWord();
	if (!_creditsSequence) {

//		const int h = 128;
		if (strId != 0xFFFF) {
			const uint8_t *str = _res->getCineString(strId);
			if (str) {
//			emu_printf("Cutscene::op_drawCaptionText() %x %s\n",strId, str);
				hasText = true;
				if(strId!=previousStr)
				{
					memset4_fast(&_vid->_frontLayer[352 << 9], 0x0000, 60 << 9); // nettoyeur de texte du bas
					drawText(0, 129, str, 0xEF, _vid->_frontLayer, kTextJustifyAlign);
				}
				previousStr = strId;
			}
		} else if (_id == kCineEspions) {
			// cutscene relies on drawCaptionText opcodes for timing
			_frameDelay = 100;
			sync(_frameDelay);
		}
		else
		{
//			emu_printf("Cutscene::op_drawCaptionText() %x clean\n",strId);
//			if(strId!=previousStr)
//			{
			memset4_fast(&_vid->_frontLayer[352 << 9], 0x0000, 60 << 9); // nettoyeur de texte du bas
			_stub->copyRect(0, 352, CLEAN_W, 60, _vid->_frontLayer, _vid->_w);
//				previousStr = strId;
//			}
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
// vbt : ne pas toucher
	updateScreen();
	clearBackPage();
// nettoyeur de texte
	memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9);
	_creditsSlowText = true;
	op_handleKeys();
}

void Cutscene::drawShapeScale(const uint8_t *data, int16_t zoom, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g) {
//	//emu_printf("Cutscene::drawShapeScale(%d, %d, %d, %d, %d, %d, %d)\n", zoom, b, c, d, e, f, g);
	_gfx.setLayer(_backPage);
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
		if (_isConcavePolygonShape) {
			_gfx.floodFill(_primitiveColor, _vertices, numVertices);
		} else {
			_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices);
		}
	}
}

void Cutscene::op_drawShapeScale() {
//	emu_printf("Cutscene::op_drawShapeScale()\n");

	_shape_count = 0;

	int16_t x = 0;
	int16_t y = 0;
	uint16_t shapeOffset = fetchNextCmdWord();
	if (shapeOffset & 0x8000) {
		x = fetchNextCmdWord();
		y = fetchNextCmdWord();
	}
	checkShape(shapeOffset);

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
	_gfx.setLayer(_backPage);
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
		if (_isConcavePolygonShape) {
			_gfx.floodFill(_primitiveColor, _vertices, numVertices);
		} else {
			_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices + 1);
		}
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
	checkShape(shapeOffset);

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
//	DMA_ScuMemCopy(_backPage, _frontPage, IMG_SIZE); // pas de dma possible
	memcpyl(_backPage, _frontPage, IMG_SIZE);

// vbt : on nettoie 	
	memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9); // nettoyeur de texte du bas
	previousStr = -1;
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

		_gfx.setLayer(_backPage);
		drawSetShape(_memoSetShape2Data, 0, (int16_t)memoSetPos[_memoSetOffset + 1], (int16_t)memoSetPos[_memoSetOffset + 2], paletteLut);
		_memoSetOffset += 3;
		if (memoSetPos[_memoSetOffset] == 4) {
			drawSetShape(_memoSetShape4Data, 0, (int16_t)memoSetPos[_memoSetOffset + 1], (int16_t)memoSetPos[_memoSetOffset + 2], paletteLut);
			_memoSetOffset += 3;
		}
	}
	*/
	updateScreen(); // vbt : remis permet de revoir "present" au boot
	// ne pas simplifier, ajoute un bug sur la sequence de fin
}

void Cutscene::op_drawTextAtPos() {

	uint16_t strId = fetchNextCmdWord();
	if (strId != 0xFFFF) {
//	emu_printf("Cutscene::op_drawTextAtPos() %x %d\n",strId, _creditsSequence);
	
		int16_t x = (int8_t)fetchNextCmdByte() * 8;
		int16_t y = (int8_t)fetchNextCmdByte() * 8;
		if (!_creditsSequence) {
			const uint8_t *str = _res->getCineString(strId & 0xFFF);
			if (str) {
				hasText = true;
//	emu_printf("Cutscene::op_drawTextAtPos() %d\n",_id);
				if(strId!=previousStr)
				{
					const uint8_t color = 0xD0 + (strId >> 0xC);

					if(!((_id >= 37 && _id <= 42) || (_id >= 66 && _id <= 71)))
						memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9);
					
					drawText(x, y, str, color, _vid->_frontLayer, kTextJustifyCenter);
					previousStr = strId;
				}
			}
			// 'voyage' - cutscene script redraws the string to refresh the screen
/*			if (_id == kCineVoyage && (strId & 0xFFF) == 0x45) {
//				emu_printf("voyage\n");
				if ((_cmdPtr - _cmdStartPtr) == 0xA) {
//					emu_printf("on doit effacer le texte ???\n");
				} else {*/
					_stub->sleep(15);
//				}
//			}
		}
	}
	else
	{
		memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9);
	}
}

void Cutscene::op_handleKeys() {
//	emu_printf("Cutscene::op_handleKeys()\n");
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
	const int16_t n = fetchNextCmdWord();
	if (n < 0) {
//		emu_printf("Cutscene::op_handleKeys n:%d\n", n);
		_stop = true;
		return;
	}
	_cmdPtr = _cmdStartPtr = getCommandData() + n + _baseOffset;
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
emu_printf("_id %d _music %d\n",_id,_musicTableDOS[_id]);
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
	if (_id != 3 && _id != 19 && _id != 22 && _id != 23 && _id != 24) {
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
				emu_printf("startOffset %d count %d num %d\n", startOffset, count, num);
			}
		}
		p += _baseOffset;
	}
	_cmdPtr = _cmdStartPtr = p + offset;
	_polPtr = getPolygonData();
//	debug(DBG_CUT, "_baseOffset = %d offset = %d count = %d", _baseOffset, offset, count);

//	_paletteNum = -1;
	_isConcavePolygonShape = false;
//	_drawMemoSetShapes = (_id == kCineMemo);
//	_memoSetOffset = 0;
	while (!_stub->_pi.quit && !_interrupted && !_stop) {
		uint8_t op = fetchNextCmdByte();
//		debug(DBG_CUT, "Cutscene::play() opcode = 0x%X (%d)", op, (op >> 2));
		if (op & 0x80) {
			break;
		}
		op >>= 2;
		if (op >= NUM_OPCODES) {
//			emu_printf("Invalid cutscene opcode = 0x%02X\n", op);
			continue;
		}
 		(this->*_opcodeTable[op])();
		_stub->processEvents();
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			_interrupted = true;
		}
	}
//		//emu_printf("VBT cutmainLoop h\n");
	_interrupted = false;
	_stop=true;
}

bool Cutscene::load(uint16_t cutName) {
//emu_printf(" Cutscene::load %d id %d\n", cutName, _id);
//	assert(cutName != 0xFFFF);
	if(cutName == 0xFFFF)
		return 0;
/*	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();
*/
	cutName &= 0xFF;
	const char *name = _namesTableDOS[cutName];
	if(cutName!=12 && cutName!=31 && cutName!=35 && cutName!=2)
	{
		_res->MAC_loadCutscene(name);
	}
	else
	{
		_res->MAC_closeMainFile();
		_res->load(name, Resource::OT_CMP);
		_res->MAC_reopenMainFile();
	}
// fix bug on displaying key cutscene
//	memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9], 0x0000, CLEAN_H << 9);
	slTVOn();	
/*	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration MAC_loadCutscene : %d\n",e-s);
*/
/*		break;
	}*/
//	e = _stub->getTimeStamp();
//emu_printf("cmd %p pol %p\n",_res->_cmd, _res->_pol);
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
// 		efface le texte sous les icones
		memset4_fast(&_vid->_frontLayer[51 << 9], 0x00,32 << 9);
//		_stub->copyRect(0, 51, _vid->_w, 32, _vid->_frontLayer, _vid->_w);
//		efface les sous-titres
		memset4_fast(&_vid->_frontLayer[CLEAN_Y << 9],0x0000, CLEAN_H_MORE << 9);
		_stub->copyRect(CLEAN_X, CLEAN_Y, CLEAN_W, CLEAN_H_MORE, _vid->_frontLayer, _vid->_w);
#endif
		Color clut[512];
//		Color *clut = (Color *)hwram_ptr;
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
	_frontPage = (uint8_t *)hwram_screen;
//	_backPage = (uint8_t *)_frontPage+IMG_SIZE;
	_backPage = (uint8_t *)SCRATCH+4096;//hwram_ptr; //SCRATCH
	_auxPage = (uint8_t *)hwram_screen+IMG_SIZE;
slTVOff();
slSynch(); // VBT : à remettre // utile
	memset4_fast(&_vid->_frontLayer[51 << 9], 0x00,32 << 9);
	_stub->copyRect(0, 51, _vid->_w, 32, _vid->_frontLayer, _vid->_w);
//emu_printf("prepare cutscene\n");
//	memset4_fast(_auxPage, 0x00, IMG_SIZE/2);
	memset4_fast(_backPage, 0x00, IMG_SIZE);
	memset4_fast(_frontPage, 0x00, IMG_SIZE+IMG_SIZE/2);
//	memset4_fast((uint8_t *)(SpriteVRAM + 0x80000 - IMG_SIZE*2), 0x00, IMG_SIZE*2);

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
	_stub->initTimeStamp();
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
//		emu_printf("Cutscene::play() _id=0x%X c%p \n", _id , current_lwram);
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
					cutName = 12; // ASC
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

void Cutscene::drawSetShape(const uint8_t *p, uint16_t offset, int x, int y) {
//emu_printf("Cutscene::drawSetShape(%p,%d,%d,%d)\n",p,offset,x,y);	
	const int count = READ_BE_UINT16(p + offset); offset += 2;
	for (int i = 0; i < count - 1; ++i) {
		offset += 5; // shape_marker
		const int verticesCount = p[offset++];
		const int ix = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
		const int iy = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
		uint8_t color = p[offset]; offset += 2;

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
	if(bgCount > kMaxShapesCount)
		return;
	
	for (int i = 0; i < bgCount; ++i) {
		uint16_t nextOffset = readSetShapeOffset(p, offset);
		backgroundShapes[i].offset = offset;
		backgroundShapes[i].size = nextOffset - offset;
		offset = nextOffset + 45;
	}
	SetShape foregroundShapes[kMaxShapesCount];
	const int fgCount = READ_BE_UINT16(p + offset); offset += 2;

	if(fgCount > kMaxShapesCount)
		return;

	for (int i = 0; i < fgCount; ++i) {
		uint16_t nextOffset = readSetShapeOffset(p, offset);
		foregroundShapes[i].offset = offset;
		foregroundShapes[i].size = nextOffset - offset;
		offset = nextOffset + 45;
	}
	prepare();
	_gfx.setLayer(_backPage);

	offset = 10;
	const int frames = READ_BE_UINT16(p + offset); offset += 2;
	transferAux = 1;
	_frameDelay = 5;
	
	for (int i = 0; i < frames && !_stub->_pi.quit && !_interrupted; ++i) {
		const int shapeBg = READ_BE_UINT16(p + offset); offset += 2;
		const int count = READ_BE_UINT16(p + offset); offset += 2;
		uint16_t paletteBuffer[16];

		if(transferAux)
		{
			readSetPalette(p, backgroundShapes[shapeBg].offset + backgroundShapes[shapeBg].size, paletteBuffer);
			copyPalette((uint8_t *)paletteBuffer, 0);
			drawSetShape(p, backgroundShapes[shapeBg].offset, 0, 0);
			packPixels(_backPage, _auxPage, IMG_SIZE);
			clearBackPage();
		}
		_gfx.setLayer(_frontPage);
		int newpal = 1;

		for (int j = 0; j < count; ++j) {
			const int shapeFg = READ_BE_UINT16(p + offset); offset += 2;
			const int shapeX = (int16_t)READ_BE_UINT16(p + offset); offset += 2;
			const int shapeY = (int16_t)READ_BE_UINT16(p + offset); offset += 2;

			if(newpal)
			{
				readSetPalette(p, foregroundShapes[shapeFg].offset + foregroundShapes[shapeFg].size, paletteBuffer);
				copyPalette((uint8_t *)paletteBuffer, 1);
				newpal = 0;
			}
			drawSetShape(p, foregroundShapes[shapeFg].offset, shapeX, shapeY);
		}
		updateScreen();
		memset4_fast(_frontPage, 0, IMG_SIZE);

		_stub->processEvents();
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			_interrupted = true;
		}
	}

	unload();
	_stop = true; // pour reprendre la musique
	
	frame_y = frame_x = 0;
	frame_z = 30;
}
