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
extern "C"
{
	#include 	<string.h>	
#include <sega_mem.h>
//#include <sega_spr.h>
//#include <sega_csh.h>
#include "sat_mem_checker.h"
}
#include "saturn_print.h"
#include <cstdint>
#include "sys.h"
#include "file.h"
#include "mixer.h"
#include "mod_player.h"
#include "util.h"

#undef assert
#define assert(x) if(!(x)){emu_printf("assert %s %d %s\n", __FILE__,__LINE__,__func__);}


volatile Uint8 slaveMixing;
volatile Uint8 slaveProceed;

ModPlayer::ModPlayer(Mixer *mixer, const char *dataPath)
	: _playing(false), _mix(mixer), _dataPath(dataPath) {
	memset(&_modInfo, 0, sizeof(_modInfo));
emu_printf("ModPlayer::ModPlayer\n");

#ifdef SLAVE_SOUND
	*(volatile Uint8*)OPEN_CSH_VAR(slaveMixing) = 0;
	*(volatile Uint8*)OPEN_CSH_VAR(slaveProceed) = 1;
#else
	slaveMixing = 0;
	slaveProceed = 1;
#endif
}

uint16 ModPlayer::findPeriod(uint16 period, uint8 fineTune) const {
	for (int p = 0; p < 36; ++p) {
		if (_periodTable[p] == period) {
			return fineTune * 36 + p;
		}
	}
	error("Invalid period=%d", period);
	return 0;
}

void ModPlayer::load(File *f) {
	f->read(_modInfo.songName, 20);
	_modInfo.songName[20] = 0;
//	debug(DBG_MOD, "songName = '%s'", _modInfo.songName);

	for (int s = 0; s < NUM_SAMPLES; ++s) {
		SampleInfo *si = &_modInfo.samples[s];
		f->read(si->name, 22);
		si->name[22] = 0;
		si->len = f->readUint16BE() * 2;
		si->fineTune = f->readByte();
		si->volume = f->readByte();
		si->repeatPos = f->readUint16BE() * 2;
		si->repeatLen = f->readUint16BE() * 2;
		si->data = 0;
		assert(si->len == 0 || si->repeatPos + si->repeatLen <= si->len);
		//debug(DBG_MOD, "sample=%d name='%s' len=%d vol=%d", s, si->name, si->len, si->volume);
	}
	_modInfo.numPatterns = f->readByte();
	assert(_modInfo.numPatterns < NUM_PATTERNS);
	f->readByte(); // 0x7F
	f->read(_modInfo.patternOrderTable, NUM_PATTERNS);
	f->readUint32BE(); // 'M.K.', Protracker, 4 channels
//#define MIN(x,y) ((x)<(y)?(x):(y))

	uint16 n = 0;
	for (int i = 0; i < NUM_PATTERNS; ++i) {
		if (_modInfo.patternOrderTable[i] != 0) {
			n = MAX(n, _modInfo.patternOrderTable[i]);
		}
	}
	//debug(DBG_MOD, "numPatterns=%d",n + 1);
	n = (n + 1) * 64 * 4 * 4; // 64 lines of 4 notes per channel
emu_printf("std_malloc in ModPlayer::load malloc %d\n",n);	
	_modInfo.patternsTable = (uint8 *)std_malloc(n);
	assert(_modInfo.patternsTable);
	f->read(_modInfo.patternsTable, n);

	for (int s = 0; s < NUM_SAMPLES; ++s) {
		SampleInfo *si = &_modInfo.samples[s];
		if (si->len != 0) {
			si->data = (int8 *)sat_malloc(si->len);
			if (si->data) {
				f->read(si->data, si->len);
			}
		}
	}
}

void ModPlayer::unload() {
//emu_printf("ModPlayer::unload\n");	
	//fprintf_saturn(stdout, "unload!");
#ifdef SLAVE_SOUND	
	// Avoid slave continuing
	*(volatile Uint8*)OPEN_CSH_VAR(slaveProceed) = 0;
	// Waiting for slave to finish
	while(*(volatile Uint8*)OPEN_CSH_VAR(slaveMixing));
#else
	slaveProceed=0;
#endif
	if (_modInfo.songName[0]) {
		sat_free(_modInfo.patternsTable);
		for (int s = 0; s < NUM_SAMPLES; ++s) {
			sat_free(_modInfo.samples[s].data);
		}
		memset(&_modInfo, 0, sizeof(_modInfo));
	}
#ifdef SLAVE_SOUND
	// Ok, slave can go on
	*(volatile Uint8*)OPEN_CSH_VAR(slaveProceed) = 1;
#else
	slaveProceed = 1;
#endif
}

void ModPlayer::handleNote(int trackNum, uint32 noteData) {
	Track *tk = &_tracks[trackNum];
	uint16 sampleNum = ((noteData >> 24) & 0xF0) | ((noteData >> 12) & 0xF);
	uint16 samplePeriod = (noteData >> 16) & 0xFFF;
	uint16 effectData = noteData & 0xFFF;
	//debug(DBG_MOD, "ModPlayer::handleNote(%d) p=%d/%d sampleNumber=0x%X samplePeriod=0x%X effectData=0x%X tk->period=%d", trackNum, _currentPatternPos, _currentPatternOrder, sampleNum, samplePeriod, effectData, tk->period);
	if (sampleNum != 0) {
		tk->sample = &_modInfo.samples[sampleNum - 1];
		tk->volume = tk->sample->volume;
		tk->pos = 0;
	}
	if (samplePeriod != 0) {
		tk->periodIndex = findPeriod(samplePeriod, tk->sample->fineTune);
		if ((effectData >> 8) != 0x3 && (effectData >> 8) != 0x5) {
			tk->period = _periodTable[tk->periodIndex];
			tk->freq = PAULA_FREQ / tk->period;
		} else {
			tk->portamento = _periodTable[tk->periodIndex];
		}
		tk->vibratoAmp = 0;
		tk->vibratoSpeed = 0;
		tk->vibratoPos = 0;
	}
	tk->effectData = effectData;
}

void ModPlayer::applyVolumeSlide(int trackNum, int amount) {
	//debug(DBG_MOD, "ModPlayer::applyVolumeSlide(%d, %d)", trackNum, amount);
	Track *tk = &_tracks[trackNum];
	int vol = tk->volume + amount;
	if (vol < 0) {
		vol = 0;
	} else if (vol > 64) {
		vol = 64;
	}
	tk->volume = vol;
}

void ModPlayer::applyVibrato(int trackNum) {
	//debug(DBG_MOD, "ModPlayer::applyVibrato(%d)", trackNum);
	Track *tk = &_tracks[trackNum];
	int vib = tk->vibratoAmp * _sineWaveTable[tk->vibratoPos] / 128;
	if (tk->period + vib != 0) {
		tk->freq = PAULA_FREQ / (tk->period + vib);
	}
	tk->vibratoPos += tk->vibratoSpeed;
	if (tk->vibratoPos >= 64) {
		tk->vibratoPos = 0;
	}
}

void ModPlayer::applyPortamento(int trackNum) {
	//debug(DBG_MOD, "ModPlayer::applyPortamento(%d)", trackNum);
	Track *tk = &_tracks[trackNum];
	if (tk->period < tk->portamento) {
		tk->period = MIN(tk->period + tk->portamentoSpeed, tk->portamento);
	} else if (tk->period > tk->portamento) {
		tk->period = MAX(tk->period - tk->portamentoSpeed, tk->portamento);
	}
	if (tk->period != 0) {
		tk->freq = PAULA_FREQ / tk->period;
	}
}

void ModPlayer::handleEffect(int trackNum, bool tick) {
	// FIXME: needed?
	//CSH_AllClr();

	Track *tk = &_tracks[trackNum];
	uint8 effectNum = tk->effectData >> 8;
	uint8 effectXY = tk->effectData & 0xFF;
	uint8 effectX = effectXY >> 4;
	uint8 effectY = effectXY & 0xF;
	//debug(DBG_MOD, "ModPlayer::handleEffect(%d) effectNum=0x%X effectXY=0x%X", trackNum, effectNum, effectXY);
	switch (effectNum) {
	case 0x0: // arpeggio
		if (tick && effectXY != 0) {
			uint16 period = tk->period;
			switch (_currentTick & 3) {
			case 1:
				period = _periodTable[tk->periodIndex + effectX];
				break;
			case 2:
				period = _periodTable[tk->periodIndex + effectY];
				break;
			}
			tk->freq = PAULA_FREQ / period;
		}
		break;
	case 0x1: // portamento up
		if (tick) {
			tk->period -= effectXY;
			if (tk->period < 113) { // note B-3
				tk->period = 113;
			}
			tk->freq = PAULA_FREQ / tk->period;
		}
		break;
	case 0x2: // portamento down
		if (tick) {
			tk->period += effectXY;
			if (tk->period > 856) { // note C-1
				tk->period = 856;
			}
			tk->freq = PAULA_FREQ / tk->period;
		}
		break;
	case 0x3: // tone portamento
		if (!tick) {
        	if (effectXY != 0) {
        		tk->portamentoSpeed = effectXY;
        	}
		} else {
			applyPortamento(trackNum);
		}
		break;
	case 0x4: // vibrato
		if (!tick) {
			if (effectX != 0) {
				tk->vibratoSpeed = effectX;
			}
			if (effectY != 0) {
				tk->vibratoAmp = effectY;
			}
		} else {
			applyVibrato(trackNum);
		}
		break;
	case 0x5: // tone portamento + volume slide
		if (tick) {
			applyPortamento(trackNum);
			applyVolumeSlide(trackNum, effectX - effectY);
		}
		break;
	case 0x6: // vibrato + volume slide
		if (tick) {
			applyVibrato(trackNum);
			applyVolumeSlide(trackNum, effectX - effectY);
		}
		break;
	case 0x9: // set sample offset
		if (!tick) {
			tk->pos = effectXY << (8 + FRAC_BITS);
		}
		break;
	case 0xA: // volume slide
		if (tick) {
			applyVolumeSlide(trackNum, effectX - effectY);
		}
		break;
	case 0xB: // position jump
		if (!tick) {
			_currentPatternOrder = effectXY;
			_currentPatternPos = 0;
			//assert(_currentPatternOrder < _modInfo.numPatterns);
		}
		break;
	case 0xC: // set volume
		if (!tick) {
			//assert(effectXY <= 64);
			tk->volume = effectXY;
		}
		break;
	case 0xD: // pattern break
		if (!tick) {
			_currentPatternPos = effectX * 10 + effectY;
			//assert(_currentPatternPos < 64);
			++_currentPatternOrder;
			//debug(DBG_MOD, "_currentPatternPos = %d _currentPatternOrder = %d", _currentPatternPos, _currentPatternOrder);
		}
		break;
	case 0xE: // extended effects
		switch (effectX) {
		case 0x0: // set filter, ignored
			break;
		case 0x1: // fineslide up
			if (!tick) {
				tk->period -= effectY;
				if (tk->period < 113) { // B-3 note
					tk->period = 113;
				}
				tk->freq = PAULA_FREQ / tk->period;
			}
			break;
		case 0x2: // fineslide down
			if (!tick) {
				tk->period += effectY;
				if (tk->period > 856) { // C-1 note
					tk->period = 856;
				}
				tk->freq = PAULA_FREQ / tk->period;
			}
			break;
		case 0x6: // loop pattern
			if (!tick) {
				if (effectY == 0) {
					_patternLoopPos = _currentPatternPos | (_currentPatternOrder << 8);
					//debug(DBG_MOD, "_patternLoopPos=%d/%d", _currentPatternPos, _currentPatternOrder);
				} else {
					if (_patternLoopCount == -1) {
						_patternLoopCount = effectY;
						_currentPatternPos = _patternLoopPos & 0xFF;
						_currentPatternOrder = _patternLoopPos >> 8;
					} else {
						--_patternLoopCount;
						if (_patternLoopCount != 0) {
							_currentPatternPos = _patternLoopPos & 0xFF;
							_currentPatternOrder = _patternLoopPos >> 8;
						} else {
							_patternLoopCount = -1;
						}
					}
					//debug(DBG_MOD, "_patternLoopCount=%d", _patternLoopCount);
				}
			}
			break;
		case 0x9: // retrigger sample
			if (tick) {
				tk->retriggerCounter = effectY;
			} else {
				if (tk->retriggerCounter == 0) {
					tk->pos = 0;
					tk->retriggerCounter = effectY;
					//debug(DBG_MOD, "retrigger sample=%d _songSpeed=%d", effectY, _songSpeed);
				}
				--tk->retriggerCounter;
			}
			break;
		case 0xA: // fine volume slide up
			if (!tick) {
				applyVolumeSlide(trackNum, effectY);
			}
			break;
		case 0xB: // fine volume slide down
			if (!tick) {
				applyVolumeSlide(trackNum, -effectY);
			}
			break;
		case 0xC: // cut sample
			if (!tick) {
				tk->cutCounter = effectY;
			} else {
				--tk->cutCounter;
				if (tk->cutCounter == 0) {
					tk->volume = 0;
				}
			}
		case 0xD: // delay sample
			if (!tick) {
				tk->delayCounter = effectY;
			} else {
				if (tk->delayCounter != 0) {
					--tk->delayCounter;
				}
			}
			break;
		case 0xE: // delay pattern
			if (!tick) {
				//debug(DBG_MOD, "ModPlayer::handleEffect() _currentTick=%d delay pattern=%d", _currentTick, effectY);
				_patternDelay = effectY;
			}
			break;
		default:
			//warning("Unhandled extended effect 0x%X params=0x%X", effectX, effectY);
			break;
		}
		break;
	case 0xF: // set speed
		if (!tick) {
			if (effectXY < 0x20) {
				_songSpeed = effectXY;
			} else {
				_songTempo = effectXY;
			}
		}
		break;
	default:
		//warning("Unhandled effect 0x%X params=0x%X", effectNum, effectXY);
		break;
	}
}

void ModPlayer::handleTick() {
	if (!_playing) {
		return;
	}
//	if (_patternDelay != 0) {
//		--_patternDelay;
//		return;
//	}
	if (_currentTick == 0) {
		//debug(DBG_MOD, "_currentPatternOrder=%d _currentPatternPos=%d", _currentPatternOrder, _currentPatternPos);
		uint8 currentPattern = _modInfo.patternOrderTable[_currentPatternOrder];
		const uint8 *p = _modInfo.patternsTable + (currentPattern * 64 + _currentPatternPos) * 16;
		for (int i = 0; i < NUM_TRACKS; ++i) {
			uint32_t noteData = READ_BE_UINT32(p);
			handleNote(i, noteData);
			p += 4;
		}
		++_currentPatternPos;
		if (_currentPatternPos == 64) {
			++_currentPatternOrder;
			_currentPatternPos = 0;
			//debug(DBG_MOD, "ModPlayer::handleTick() _currentPatternOrder = %d/%d", _currentPatternOrder, _modInfo.numPatterns);
			// On the amiga version, the introduction cutscene is shorter than the PC version ;
			// so the music module doesn't synchronize at all with the PC datafiles, here we
			// add a hack to let the music play longer
			if (_songNum == 0 && _currentPatternOrder == 3 && !_introSongHack) {
				_currentPatternOrder = 1;
				_introSongHack = true;
				//warning("Introduction module synchronization hack");
			}
		}
	}
	for (int i = 0; i < NUM_TRACKS; ++i) {
		handleEffect(i, (_currentTick != 0));
	}
	++_currentTick;
	if (_currentTick == _songSpeed) {
		_currentTick = 0;
	}
	if (_currentPatternOrder == _modInfo.numPatterns) {
		//debug(DBG_MOD, "ModPlayer::handleEffect() _currentPatternOrder == _modInfo.numPatterns");
		_playing = false;
	}
}

void ModPlayer::mixSamples(int8 *buf, int samplesLen) {
	for (int i = 0; i < NUM_TRACKS; ++i) {
		Track *tk = &_tracks[i];
		if (tk->sample != 0 && tk->delayCounter == 0) {
			int8 *mixbuf = buf;
			SampleInfo *si = tk->sample;
			int len = si->len << FRAC_BITS;
			int loopLen = si->repeatLen << FRAC_BITS;
			int loopPos = si->repeatPos << FRAC_BITS;
			int deltaPos = (tk->freq << FRAC_BITS) / /*_mix->getSampleRate()*/22050;
			int curLen = samplesLen;
			int pos = tk->pos;
			while (curLen != 0) {
				int count;
				if (loopLen > (2 << FRAC_BITS)) {
					if (pos >= loopPos + loopLen) {
						pos -= loopLen;
					}
					count = MIN(curLen, (loopPos + loopLen - pos - 1) / deltaPos + 1);
					curLen -= count;
				} else {
					if (pos >= len) {
						count = 0;
					} else {
						count = MIN(curLen, (len - pos - 1) / deltaPos + 1);
					}
					curLen = 0;
				}
				while (count--) {
					//int out = resample3Pt(si, pos, deltaPos, FRAC_BITS);
					int out = resampleLinear(si, pos, deltaPos, FRAC_BITS);
					Mixer::addclamp(*mixbuf++, out * tk->volume / 64);
					pos += deltaPos;
				}
			}
			tk->pos = pos;
     	}
	}
}

bool ModPlayer::mix(int8 *buf, int len) {
//emu_printf("ModPlayer::mix %p %d\n",buf, len);		
#ifdef SLAVE_SOUND
	while(!(*(volatile Uint8*)OPEN_CSH_VAR(slaveProceed))); // Wait that we are safe and able to proceed
	*(volatile Uint8*)OPEN_CSH_VAR(slaveMixing) = 1; // Proceed...
#else
	slaveMixing = 1;	// vbt passagr à 0
#endif
	if (_playing) {
		//memset(buf, 0, len);
		const int samplesPerTick = _mix->getSampleRate() / (50 * _songTempo / 125);
		while (len != 0) {
			if (_samplesLeft == 0) {
				handleTick();
				_samplesLeft = samplesPerTick;
			}
			int count = _samplesLeft;
			if (count > len) {
				count = len;
			}
			_samplesLeft -= count;
			len -= count;
			mixSamples(buf, count);
			buf += count;
		}
	}
#ifdef SLAVE_SOUND
	*(volatile Uint8*)OPEN_CSH_VAR(slaveMixing) = 0;
#else
	slaveMixing = 0;
#endif
	return _playing;
}

void ModPlayer::play(int num, int tempo) {
emu_printf("ModPlayer::play(%d, %d)\n",num,tempo);
#if 0
	if (!_playing && num < _modulesFilesCount) {
		File f;
		bool found = false;
		for (uint8 i = 0; i < ARRAYSIZE(_modulesFiles[num]); ++i) {
			if (f.open(_modulesFiles[num][i], _dataPath, "rb")) {
				found = true;
				break;
			}
		}
		if (!found) {
			emu_printf("Can't find music file %d %s\n", num,_modulesFiles[num][1]);
		} else {
			emu_printf("music file found %d %s\n", num,_modulesFiles[num][1]);			
			load(&f);
			_currentPatternOrder = 0;
			_currentPatternPos = 0;
			_currentTick = 0;
			_patternDelay = 0;
			_songSpeed = 6;
			_songTempo = 125;
			_patternLoopPos = 0;
			_patternLoopCount = -1;
			_samplesLeft = 0;
			_songNum = num;
			_introSongHack = false;
			memset(_tracks, 0, sizeof(_tracks));
			_mix->setPremixHook(mixCallback, this);
			_playing = true;
		}
	}
#endif
}

void ModPlayer::stop() {
	if (_playing) {
		_mix->setPremixHook(0, 0);
		_playing = false;
	}
	unload();
}

bool ModPlayer::mixCallback(void *param, int8 *buf, int len) {
	return ((ModPlayer *)param)->mix(buf, len);
}
