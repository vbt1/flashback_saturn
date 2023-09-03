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

#ifndef __CUTSCENE_H__
#define __CUTSCENE_H__

#include "intern.h"
#include "graphics.h"

struct ModPlayer;
struct Resource;
struct SystemStub;
struct Video;

struct Cutscene {
	typedef void (Cutscene::*OpcodeStub)();

	enum {
		MAX_VERTICES = 128,
		NUM_OPCODES = 15
	};

	enum {
		kTextJustifyLeft = 0,
		kTextJustifyAlign = 1,
		kTextJustifyCenter = 2,
	};

	enum {
		kCineDebut = 0,
		kCineChute = 47,
		kCineMemo = 48,
		kCineVoyage = 52,
		kCineEspions = 57
	};

	struct SetShape {
		uint16_t offset;
		uint16_t size;
	};

	struct Text {
		int num;
		const char *str;
	};

	static const OpcodeStub _opcodeTable[];
	static const char *_namesTableDOS[];
	static const uint16_t _offsetsTableDOS[];
	static const uint16_t _cosTable[];
	static const uint16_t _sinTable[];
	static const uint8_t _creditsDataDOS[];
	static const uint16_t _creditsCutSeq[];
	static const uint8 _musicTable[];
//	static const uint8 _protectionShapeData[];

	Graphics _gfx;
	ModPlayer *_ply;
	Resource *_res;
	SystemStub *_stub;
	Video *_vid;
	const uint8_t *_patchedOffsetsTable;

	uint16_t _id;
	uint16_t _deathCutsceneId;
	bool _interrupted;
	bool _stop;
	const uint8_t *_polPtr;
	const uint8_t *_cmdPtr;
	const uint8_t *_cmdPtrBak;
	uint32_t _tstamp;
	uint8_t _frameDelay;
	bool _newPal;
	uint8_t _palBuf[16 * sizeof(uint16_t) * 2];
	uint16_t _baseOffset;
	bool _creditsSequence;
	uint32 _rotData[4];
	uint8_t _primitiveColor;
	uint8_t _clearScreen;
	Point _vertices[MAX_VERTICES];
	bool _hasAlphaColor;
	uint8_t _varKey;
	int16_t _shape_ix;
	int16_t _shape_iy;
	int16_t _shape_ox;
	int16_t _shape_oy;
	int16_t _shape_cur_x;
	int16_t _shape_cur_y;
	int16_t _shape_prev_x;
	int16_t _shape_prev_y;
	uint16_t _shape_count;
	uint32_t _shape_cur_x16;
	uint32_t _shape_cur_y16;
	uint32_t _shape_prev_x16;
	uint32_t _shape_prev_y16;
	uint8_t _textSep[0x14];
	uint8_t _textBuf[500];
	const uint8_t *_textCurPtr;
	uint8_t *_textCurBuf;
	bool _creditsSlowText;
	bool _creditsKeepText;
	uint8_t _creditsTextPosX;
	uint8_t _creditsTextPosY;
	int16_t _creditsTextCounter;
	int _creditsTextIndex; /* MAC has the credits data in a resource */
	int _creditsTextLen;
	uint8_t *_frontPage, *_backPage, *_auxPage;

	Cutscene(ModPlayer *player, Resource *res, SystemStub *stub, Video *vid);
	const uint8_t *getCommandData() const;
	const uint8_t *getPolygonData() const;

	void sync();
	void copyPalette(const uint8 *palo, uint16 num);
	void updatePalette();
	void setPalette();
	void initRotationData(uint16 a, uint16 b, uint16 c);
	uint16 findTextSeparators(const uint8 *p);
	void drawText(int16 x, int16 y, const uint8 *p, uint16 color, uint8 *page, uint8 n);
	void swapLayers();
	void drawCreditsText();
//	void drawProtectionShape(uint8 shapeNum, int16 zoom);
	void drawShape(const uint8 *data, int16 x, int16 y);
	void drawShapeScale(const uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g);
	void drawShapeScaleRotate(const uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g);

	void op_markCurPos();
	void op_refreshScreen();
	void op_waitForSync();
	void op_drawShape();
	void op_setPalette();
	void op_drawStringAtBottom();
	void op_nop();
	void op_skip3();
	void op_refreshAll();
	void op_drawShapeScale();
	void op_drawShapeScaleRotate();
	void op_drawCreditsText();
	void op_drawStringAtPos();
	void op_handleKeys();

	uint8 fetchNextCmdByte();
	uint16 fetchNextCmdWord();
	void mainLoop(uint16 offset);
	bool load(uint16_t cutName);
	void unload();
	void prepare();
	void startCredits();
	void play();
};

#endif // __CUTSCENE_H__
