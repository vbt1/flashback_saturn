/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#undef sleep 

#ifndef __SYSTEMSTUB_H__
#define __SYSTEMSTUB_H__

#include "intern.h"

#undef sleep 

//#define PCM_ADDR ((void*)0x25a20000)
//#define PCM_SIZE (4096L*8)

int  cdUnlock(void); // CD Drive unlocker, when loading game through PAR

struct PlayerInput {
	enum {
		DIR_UP    = 1 << 0,
		DIR_DOWN  = 1 << 1,
		DIR_LEFT  = 1 << 2,
		DIR_RIGHT = 1 << 3
	};
	enum {
		DF_FASTMODE = 1 << 0,
		DF_DBLOCKS  = 1 << 1,
		DF_SETLIFE  = 1 << 2,
		DF_AUTOZOOM = 1 << 3
	};
	volatile uint8 dirMask;
	volatile bool enter;
	volatile bool space;
	volatile bool shift;
	volatile bool backspace;
	volatile bool escape;
	volatile bool ltrig;
	volatile bool rtrig;

	char lastChar;

	bool save;
	bool load;
	int stateSlot;

	bool inpRecord;
	bool inpReplay;

	bool mirrorMode;

	uint8 dbgMask;
	bool quit;
};

struct SystemStub {
	typedef void (*AudioCallback)(void *param, uint8 *stream, int len);
	PlayerInput _pi;

	virtual ~SystemStub() {}

	virtual void init(const char *title, uint16 w, uint16 h) = 0;
	virtual void destroy() = 0;

	virtual void setPaletteEntry(uint16 i, const Color *c) = 0;
	virtual void getPaletteEntry(uint16 i, Color *c) = 0;
	virtual void setOverscanColor(uint8 i) = 0;
	virtual void copyRect(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;
//	virtual void copyRect2(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;
	virtual void updateScreen(uint8 shakeOffset) = 0;
//	virtual void copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb) = 0;

	virtual void processEvents() = 0;
	virtual void sleep(uint32 duration) = 0;
	virtual uint32 getTimeStamp() = 0;
	virtual void initTimeStamp();
	
	virtual void startAudio(AudioCallback callback, void *param) = 0;
	virtual void stopAudio() = 0;
	virtual uint32 getOutputSampleRate() = 0;

//	virtual void *createMutex() = 0;
	virtual void destroyMutex(void *mutex) = 0;
	virtual void lockMutex(void *mutex) = 0;
	virtual void unlockMutex(void *mutex) = 0;
	
	virtual void setPalette(uint8 *palette, uint16 colors) = 0;
	virtual void setup_input (void) = 0;
};

extern SystemStub *SystemStub_SDL_create();

#endif // __SYSTEMSTUB_H__
