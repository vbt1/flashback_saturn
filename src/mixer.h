
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef __MIXER_H__
#define __MIXER_H__

#include "intern.h"
#include "mod_player.h"

struct MixerChunk {
	uint8_t *data;
	uint32_t len;
/*
	MixerChunk()
		: data(0), len(0) {
	}*/

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
	void pauseMusic(void);
	void unpauseMusic(void);
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
