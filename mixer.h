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

#ifndef __MIXER_H__
#define __MIXER_H__

#include "intern.h"

struct MixerChunk {
	const uint8_t *data;
	uint32_t len;

	int8_t getPCM(int offset) const {
		if (offset < 0) {
			offset = 0;
		} else if (offset >= (int)len) {
			offset = len - 1;
		}
		return (int8_t)data[offset];
	}
};

struct MixerChannel {
	uint8_t active;
	uint8_t volume;
	MixerChunk chunk;
	uint32_t chunkPos;
	uint32_t chunkInc;
};

struct SystemStub;

struct Mixer {
	typedef bool (*PremixHook)(void *userData, int8 *buf, int len);

	enum MusicType {
		MT_NONE,
		MT_MOD,
		MT_OGG,
		MT_PRF,
		MT_SFX,
		MT_CPC,
	};

	enum {
		MUSIC_TRACK = 1000,
		NUM_CHANNELS = 4,
		FRAC_BITS = 12,
		MAX_VOLUME = 64
	};

	void *_mutex;
	SystemStub *_stub;
	MixerChannel _channels[NUM_CHANNELS];
	PremixHook _premixHook;
	void *_premixHookData;
	MusicType _backgroundMusicType;
	MusicType _musicType;
	int _musicTrack;

	Mixer(SystemStub *stub);
	void init();
	void free();
	void setPremixHook(PremixHook premixHook, void *userData);
	void play(const MixerChunk *mc, uint16 freq, uint8 volume);
	void stopAll();
	void playMusic(int num, int tempo = 0);
	void stopMusic();
	uint32 getSampleRate() const;
	void mix(int8 *buf, int len);

	static void addclamp(int8 &a, int b);
	static void mixCallback(void *param, uint8 *buf, int len);
};

template <class T>
int resampleLinear(T *sample, int pos, int step, int fracBits) {
	const int inputPos = pos >> fracBits;
	const int inputFrac = (1 << fracBits) - 1;
	int out = sample->getPCM(inputPos);
	out += (sample->getPCM(inputPos + 1) - out) * inputFrac >> fracBits;
	return out;
}

template <class T>
int resample3Pt(T *sample, int pos, int step, int fracBits) {
	const int inputPos = pos >> fracBits;
	const int inputFrac = (1 << fracBits) - 1;
	int out = sample->getPCM(inputPos) >> 1;
	out += sample->getPCM(inputPos + ((inputFrac - (step >> 1)) >> fracBits)) >> 2;
	out += sample->getPCM(inputPos + ((inputFrac + (step >> 1)) >> fracBits)) >> 2;
	return out;
}

#endif // __MIXER_H__
