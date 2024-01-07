
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C" {
	#include 	<string.h>
#include <sl_def.h>
#include <sega_cdc.h>

}
#include "mixer.h"
#include "systemstub.h"
#include "saturn_print.h"
/* CDDA */
extern CdcPly	playdata;
extern CdcPos	posdata;
extern CdcStat  statdata;

Mixer::Mixer(SystemStub *stub)
	: _stub(stub) {
}

void Mixer::init() {
	
//slPrint((char *)"mix.memset    ",slLocate(10,12));	
	memset(_channels, 0, sizeof(_channels));
	_premixHook = 0;
//slPrint((char *)"mix.createMutex    ",slLocate(10,12));		
	_mutex = _stub->createMutex();
//slPrint((char *)"mix.startAudio    ",slLocate(10,12));	
//slSynch();
	_stub->startAudio(Mixer::mixCallback, this);
//slPrint((char *)"mix.started Audio    ",slLocate(10,12));	
}

void Mixer::free() {
	stopAll();
	_stub->stopAudio();
	_stub->destroyMutex(_mutex);
}

void Mixer::setPremixHook(PremixHook premixHook, void *userData) {
	debug(DBG_SND, "Mixer::setPremixHook()");
	MutexStack(_stub, _mutex);
	_premixHook = premixHook;
	_premixHookData = userData;
}

void Mixer::play(const MixerChunk *mc, uint16 freq, uint8 volume) {
	//emu_printf("Mixer::play(%d, %d)\n", freq, volume);
	MutexStack(_stub, _mutex);
	MixerChannel *ch = 0;
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *cur = &_channels[i];
		if (cur->active) {
			if (cur->chunk.data == mc->data) {
				cur->chunkPos = 0;
				return;
			}
		} else {
			ch = cur;
			break;
		}
	}
	if (ch) {
		ch->active = true;
		ch->volume = volume;
		ch->chunk = *mc;
		ch->chunkPos = 0;
		ch->chunkInc = (freq << FRAC_BITS) / _stub->getOutputSampleRate();
	}
}

uint32 Mixer::getSampleRate() const {
	return _stub->getOutputSampleRate();
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");
	MutexStack(_stub, _mutex);
	for (uint8 i = 0; i < NUM_CHANNELS; ++i) {
		_channels[i].active = false;
	}
}

void Mixer::pauseMusic(void)
{
//	CDC_POS_PTYPE(&posdata)=CDC_PTYPE_NOCHG;
	CDC_CdSeek(&posdata);
//	CDC_GetCurStat(&statdata);
}

void Mixer::unpauseMusic(void)
{
/*
	CDC_PLY_STYPE(&playdata) = CDC_PTYPE_NOCHG;
	CDC_PLY_ETYPE(&playdata) = CDC_PTYPE_NOCHG;
	CDC_PLY_PMODE(&playdata) = CDC_PM_NOCHG;
*/
//	memcpy(&playdata.start,&posdata,sizeof(CdcPos));

/*
    CDC_PLY_STYPE(&playdata) = CDC_PTYPE_FAD;
    CDC_PLY_SFAD(&playdata) = statdata.report.fad;
    CDC_PLY_ETYPE(&playdata) = CDC_PTYPE_FAD;
//    CDC_PLY_EFAS(&playdata) = efad - sfad + 1;
    CDC_PLY_PMODE(&playdata) = CDC_PM_DFL;
*/
	CDC_POS_PTYPE(&posdata)=CDC_PTYPE_FAD;
	CDC_CdSeek(&posdata);	
	CDC_CdPlay(&playdata);
}
/*
        CDC_PLY_STYPE(&ply) = CDC_PTYPE_FAD;
        CDC_PLY_SFAD(&ply) = sfad;
        CDC_PLY_ETYPE(&ply) = CDC_PTYPE_FAD;
        CDC_PLY_EFAS(&ply) = efad - sfad + 1;
        CDC_PLY_PMODE(&ply) = CDC_PM_DFL;
*/

void Mixer::playMusic(int num, int tempo) {
	emu_printf("Mixer::playMusic(%d, %d)\n", num, tempo);
	int trackNum = -1;
	if (num == 1) { // menu screen
		trackNum = 2;
	} else if (num >= MUSIC_TRACK) {
		trackNum = 3+ num - MUSIC_TRACK;
	}
	emu_printf("Mixer::trackNum(%d)\n", trackNum);

	if(trackNum>1 && trackNum<10)
	{
		CDC_POS_PTYPE( &posdata ) = CDC_PTYPE_TNO;
		CDC_PLY_STNO( &playdata ) = (Uint8) (trackNum);
		CDC_PLY_ETNO( &playdata ) = (Uint8) (trackNum);
		CDC_CdPlay(&playdata);	
	}
/*	
	if (trackNum != -1 && trackNum != _musicTrack) {
		if (_ogg.playTrack(trackNum)) {
			_backgroundMusicType = _musicType = MT_OGG;
			_musicTrack = trackNum;
			return;
		}
		if (_cpc.playTrack(trackNum)) {
			_backgroundMusicType = _musicType = MT_CPC;
			_musicTrack = trackNum;
			return;
		}
	}
	if ((_musicType == MT_OGG || _musicType == MT_CPC) && isMusicSfx(num)) { // do not play level action music with background music
		return;
	}
	if (isMusicSfx(num)) { // level action sequence
		_sfx.play(num);
		if (_sfx._playing) {
			_musicType = MT_SFX;
		}
	} else { // cutscene
		_mod.play(num, tempo);
		if (_mod._playing) {
			_musicType = MT_MOD;
			return;
		}
		if (g_options.use_prf_music) {
			_prf.play(num);
			if (_prf._playing) {
				_musicType = MT_PRF;
				return;
			}
		}
	}
*/	
}

void Mixer::stopMusic() {
	emu_printf( "Mixer::stopMusic() %d\n",_musicType);
//	CDC_POS_PTYPE( &posdata ) = CDC_PTYPE_DFL;	/* Stop Music. */
/*	
	switch (_musicType) {
	case MT_NONE:
		break;
	case MT_MOD:
		_mod.stop();
		break;
	case MT_OGG:
		_ogg.pauseTrack();
		break;
	case MT_PRF:
		_prf.stop();
		break;
	case MT_SFX:
		_sfx.stop();
		break;
	case MT_CPC:
		_cpc.pauseTrack();
		break;
	}
	_musicType = MT_NONE;
	if (_musicTrack > 2) { // do not resume menu music
		switch (_backgroundMusicType) {
		case MT_OGG:
			_ogg.resumeTrack();
			_musicType = MT_OGG;
			break;
		case MT_CPC:
			_cpc.resumeTrack();
			_musicType = MT_CPC;
			break;
		default:
			break;
		}
	} else {
		_musicTrack = -1;
	}
*/	
}
void Mixer::mix(int8 *buf, int len) {
//emu_printf(" Mixer::mix\n");
	//MutexStack(_stub, _mutex);
	memset(buf, 0, len);
	if (_premixHook) {
		if (!_premixHook(_premixHookData, buf, len)) {
			_premixHook = 0;
			_premixHookData = 0;
		}
	}

	for (uint8 i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		if (ch->active) {
			for (int pos = 0; pos < len; ++pos) {
				if ((ch->chunkPos >> FRAC_BITS) >= (ch->chunk.len - 1)) {
					ch->active = false;
					break;
				}
				int out = resampleLinear(&ch->chunk, ch->chunkPos, ch->chunkInc, FRAC_BITS);
				addclamp(buf[pos], out * ch->volume / Mixer::MAX_VOLUME);
				ch->chunkPos += ch->chunkInc;
			}
		}
	}
}

void Mixer::addclamp(int8& a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	} else if (add > 127) {
		add = 127;
	}
	a = add;
}

void Mixer::mixCallback(void *param, uint8 *buf, int len) {
	((Mixer *)param)->mix((int8 *)buf, len);
}
