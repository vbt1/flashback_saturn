
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C" {
#include <string.h>
#include <sl_def.h>
#include <sega_cdc.h>
#include "cdtoc.h"
Bool GFCD_WaitScdqFlag(void);
}
#include "mixer.h"
#include "systemstub.h"
/* CDDA */
extern CDTableOfContents toc;
//extern CdcPly	playdata;
//extern CdcPos	posdata;
extern CdcStat  statdata;

Mixer::Mixer(SystemStub *stub)
	: _stub(stub) {
}

void Mixer::init() {
	
//slPrint((char *)"mix.memset    ",slLocate(10,12));	
//	memset(_channels, 0, sizeof(_channels));
//	_premixHook = 0;
//slPrint((char *)"mix.createMutex    ",slLocate(10,12));		
//	_mutex = _stub->createMutex();
//slPrint((char *)"mix.startAudio    ",slLocate(10,12));	
//slSynch();
//	_stub->startAudio(Mixer::mixCallback, this);
//slPrint((char *)"mix.started Audio    ",slLocate(10,12));	
}

void Mixer::free() {
//	setPremixHook(0, 0);
	//stopAll();
//	_stub->stopAudio();
//	_stub->destroyMutex(_mutex);
}
/*
void Mixer::setPremixHook(PremixHook premixHook, void *userData) {
	debug(DBG_SND, "Mixer::setPremixHook()");
	_premixHook = premixHook;
	_premixHookData = userData;
}
*/
uint32 Mixer::getSampleRate() const {
	return _stub->getOutputSampleRate();
}

void Mixer::pauseMusic(void)
{
	// Get current address
	CDC_GetCurStat(&statdata);
}

void Mixer::unpauseMusic(void)
{
//	emu_printf( "Mixer::unpauseMusic() _musicTrack %d %d\n",_musicTrack,statdata.report.fad);

	CdcPly ply;

    CDC_PLY_STYPE(&ply) = CDC_PTYPE_FAD; // track number

	if (statdata.report.fad == 0)
	{
		CDC_PLY_SFAD(&ply) = toc.Tracks[_musicTrack].fad;
	}
	else
	{
		CDC_PLY_SFAD(&ply) = statdata.report.fad;
	}

	if (_musicTrack + 1 < CD_TRACK_COUNT && CDTrackIsAudio(&toc.Tracks[_musicTrack + 1]))
	{
		// End of the playback address
    	CDC_PLY_ETYPE(&ply) = CDC_PTYPE_FAD;
		CDC_PLY_EFAS(&ply) = toc.Tracks[_musicTrack + 1].fad - toc.Tracks[_musicTrack].fad;
	}
	else
	{
		// End of the playback is end of the disk
    	CDC_PLY_ETYPE(&ply) = CDC_PTYPE_DFL;
	}

	// Playback mode
    CDC_PLY_PMODE(&ply) = CDC_PM_DFL | 0xf; // 0xf = infinite repetitions

	// Start playback
    CdcPos poswk;
    CDC_POS_PTYPE(&poswk) = CDC_PTYPE_DFL;
    CDC_CdSeek(&poswk);
    CDC_CdPlay(&ply);
}

void Mixer::playMusic(int num, int tempo) {
//	emu_printf("Mixer::playMusic(%d, %d)\n", num, tempo);
	int trackNum = -1;
	if (num == 1) { // menu screen
		trackNum = 2;
	} else if (num >= MUSIC_TRACK) {
		trackNum = 2 + num - MUSIC_TRACK;

		if(trackNum == 7 || trackNum == 8)
			trackNum = 6;
	}
	else
		trackNum = num;
//	emu_printf("Mixer::trackNum(%d)\n", trackNum);

	if(trackNum>1 && trackNum<40)
	{
		CdcPly ply;
		_musicTrack = trackNum + 1;
//		statdata.report.fad = 0;

		CDC_PLY_STYPE(&ply) = CDC_PTYPE_FAD; // track number
		CDC_PLY_SFAD(&ply) = toc.Tracks[trackNum].fad;

		if (trackNum + 1 < CD_TRACK_COUNT && CDTrackIsAudio(&toc.Tracks[trackNum + 1]))
		{
			// End of the playback address
			CDC_PLY_ETYPE(&ply) = CDC_PTYPE_FAD;
			CDC_PLY_EFAS(&ply) = toc.Tracks[trackNum + 1].fad - toc.Tracks[trackNum].fad;
		}
		else
		{
			// End of the playback is end of the disk
			CDC_PLY_ETYPE(&ply) = CDC_PTYPE_DFL;
		}

		// Playback mode
		CDC_PLY_PMODE(&ply) = CDC_PM_DFL | 0xf; // 0xf = infinite repetitions

		// Start playback
		CDC_CdPlay(&ply);

	}
/*	else { // cutscene
//		_mod.play(num, tempo);
//		emu_printf("cutscene MT_MOD %d\n", num);	
		if (_mod._playing) 
		{
			_musicType = MT_MOD;
			return;
		}


	}*/
}

void Mixer::stopMusic(uint8 current) {
	// Get current address
	if (!current)
	{
		CdcStat stat;
		CDC_GetCurStat(&stat);
	}
	else
	{
		CDC_GetCurStat(&statdata);
	}
//	emu_printf( "Mixer::stopMusic(%d) _musicTrack %d %d\n",current, _musicTrack,statdata.report.fad);
	// Restore address
    CdcPos poswk;
    CDC_POS_PTYPE(&poswk) = CDC_PTYPE_DFL;
    CDC_CdSeek(&poswk);
}
#if 0
void Mixer::stopMusic() {
	// Get current address
	CdcStat stat;
	CDC_GetCurStat(&stat);
	emu_printf( "Mixer::stopMusic() _musicTrack %d %d\n",_musicTrack,statdata.report.fad);

	// Restore address
    CdcPos poswk;
    CDC_POS_PTYPE(&poswk) = CDC_PTYPE_DFL;
    CDC_CdSeek(&poswk);
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
	}*/
/*	
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
#endif
/*
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
*/