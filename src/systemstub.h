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

#undef sleep 

#ifndef __SYSTEMSTUB_H__
#define __SYSTEMSTUB_H__

#include "intern.h"

#undef sleep 

//#define SLAVE_SOUND 1
#define PCM_ADDR ((void*)0x25a20000)
#define PCM_SIZE (4096L*8)

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

	virtual void setPaletteEntry(uint8 i, const Color *c) = 0;
	virtual void getPaletteEntry(uint8 i, Color *c) = 0;
	virtual void setOverscanColor(uint8 i) = 0;
	virtual void copyRect(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;
//	virtual void copyRect2(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;
	virtual void updateScreen(uint8 shakeOffset) = 0;
	virtual void SAT_updatePalette();
//	virtual void copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb) = 0;

	virtual void processEvents() = 0;
	virtual void sleep(uint32 duration) = 0;
	virtual uint32 getTimeStamp() = 0;

	virtual void startAudio(AudioCallback callback, void *param) = 0;
	virtual void stopAudio() = 0;
	virtual uint32 getOutputSampleRate() = 0;

	virtual void *createMutex() = 0;
	virtual void destroyMutex(void *mutex) = 0;
	virtual void lockMutex(void *mutex) = 0;
	virtual void unlockMutex(void *mutex) = 0;

	virtual void setPalette(uint8 *palette, uint16 colors) = 0;
	virtual void setup_input (void) = 0;
};

struct MutexStack {
	SystemStub *_stub;
	void *_mutex;

	MutexStack(SystemStub *stub, void *mutex)
		: _stub(stub), _mutex(mutex) {
		_stub->lockMutex(_mutex);
	}
	~MutexStack() {
		_stub->unlockMutex(_mutex);
	}
};

extern SystemStub *SystemStub_SDL_create();

#endif // __SYSTEMSTUB_H__
