#define PRELOAD_MONSTERS 1
//#define USE_SLAVE 1
//#define VIDEO_PLAYER 1
#define DEBUG 1
#define DEMO 1
#define BPP8 1

#define HWRAM 1<<0
#define LWRAM 1<<1
#define LWRA1 1<<2
#define VRAM1 1<<3

//#define DEBUG2 1
//#define REDUCE_4BPP 1
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C" {
	#include 	<string.h>
	#include <stdio.h>
//#include "sega_dma.h"
#include <sl_def.h>
#include <sgl.h>
#include <sega_bup.h>
#include <sega_per.h>
#include <sega_gfs.h> 
#include <sega_tim.h> 
//#include <sega_spr.h>
#include "pcm.h"
#include "scsp.h"
#include "sat_mem_checker.h"
#define	BUP_LIB_ADDRESS		(*(volatile Uint32 *)(0x6000350+8))
#define	BUP_VECTOR_ADDRESS	(*(volatile Uint32 *)(0x6000350+4))

#define	BUP_Init	((void (*)(volatile Uint32 *lib,Uint32 *work,BupConfig tp[3])) (BUP_LIB_ADDRESS))
#define	BUP_Format	((Sint32 (*)(Uint32 device)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+8)))
#define	BUP_Stat	((Sint32 (*)(Uint32 device,Uint32 datasize,BupStat *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+12)))
#define	BUP_Write	((Sint32 (*)(Uint32 device,BupDir *tb,volatile Uint8 *data,Uint8 wmode)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+16)))
#define	BUP_Read	((Sint32 (*)(Uint32 device,Uint8 *filename,volatile Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+20)))
#define	BUP_Delete	((Sint32 (*)(Uint32 device,Uint8 *filename)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+24)))
#define	BUP_Dir 	((Sint32 (*)(Uint32 device,Uint8 *filename,Uint16 tbsize,BupDir *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+28)))
#define	BUP_Verify	((Sint32 (*)(Uint32 device,Uint8 *filename,volatile Uint8 *data)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+32)))
#define	BUP_SetDate	((Uint32 (*)(BupDate *tb)) (*(Uint32 *)(BUP_VECTOR_ADDRESS+40)))
#define TVSTAT	(*(Uint16 *)0x25F80004)

extern Uint8 *current_lwram;
extern Uint8 *save_current_lwram;
extern Uint8 *soundAddr;
extern volatile Uint32 ticker;
extern volatile Uint8  tick_wrap;
extern Uint8 tickPerVblank;
unsigned char frame_x = 0;
unsigned char frame_y = 0;
unsigned char frame_z = 0;
void	*malloc(size_t);
}
extern void sat_restart_audio(void);

//extern volatile Uint8 audioEnabled;
#include "saturn_print.h"
#include "lz.h"

#include <ctime>
#include "decode_mac.h"
#include "file.h"
#include "systemstub.h"
#include "unpack.h"
#include "game.h"
#include "sys.h"

#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
extern "C" {
extern CdcStat  statdata;
extern Uint8 *hwram;
extern Uint8 *hwram_ptr;
extern Uint8 *hwram_screen;
extern Uint32 position_vram;
extern Uint32 position_vram_aft_monster;
extern unsigned int end1;
}
static SAVE_BUFFER sbuf;
/* *** */
static Uint32 getFreeSaveBlocks(void);
static void	clearSaveSlots(Uint8 level); // Clear all save slots except the one of the specified level

static Uint32 getFreeSaveBlocks(void) {
	BupConfig conf[3];
	BupStat sttb;

//    Uint32 libBakBuf[4096];
//    Uint32 BackUpRamWork[2048];
	Uint32 *libBakBuf    = (Uint32 *)(hwram_screen+SAV_BUFSIZE);
	Uint32 *BackUpRamWork= (Uint32 *)(hwram_screen+SAV_BUFSIZE+0x4000);

	PER_SMPC_RES_DIS(); // Disable reset
		BUP_Init(libBakBuf, BackUpRamWork, conf);
		if( BUP_Stat(0, 0, &sttb) == BUP_UNFORMAT) BUP_Format(0);
	PER_SMPC_RES_ENA(); // Enable reset
	
	return sttb.freeblock;
}
/*
inline void timeTick() {
	if(ticker > (0xFFFFFFFF - tickPerVblank)) {
		tick_wrap ^= 1;
		ticker = 0;
	} else {
		ticker += tickPerVblank;
	}
}
*/

/* *** */

Game::Game(SystemStub *stub, const char *dataPath, const char *savePath, int level, ResourceType ver, Language lang)
	: _cut(&_res, stub, &_vid), _menu(&_res, stub, &_vid),
	_mix(stub), _res(dataPath, ver, lang), /*_sfxPly(&_mix),*/ _vid(&_res, stub),
	_stub(stub)/*, _savePath(savePath)*/ {
	_stateSlot = 1;
	_inp_demPos = 0;
	_skillLevel = _menu._skill = kSkillNormal;
	_currentLevel = _menu._level = level;
#ifdef DEMO
	_demoBin = -1;
#endif
//	_widescreenMode = widescreenMode;
//	_autoSave = autoSave;
//	_rewindPtr = -1;
//	_rewindLen = 0;
//	_cheats = cheats;
//	_cheats = kCheatOneHitKill | kCheatNoHit | kCheatOneHitKill;
}

void Game::run() {

	_stub->init("REminiscence", Video::GAMESCREEN_W*2, Video::GAMESCREEN_H*2);
//	slIntFunction(vblIn); // Function to call at each vblank-in // vbt à remettre

	_randSeed = time(0);
	_mix.init();  // vbt : evite de fragmenter la ram	
	_res.init();   // vbt : ajout pour la partie mac

//		end1 = 584000+28000+HWRAM_SCREEN_SIZE; // vbt : marge de 20ko environ
		end1 = 584000+24000+HWRAM_SCREEN_SIZE; // vbt : marge de 20ko environ
	
		hwram = (Uint8 *)malloc(end1);//(282344);
//		memset(hwram,0x00,end1);
		end1 += (int)hwram;
		emu_printf("hwram ****%p*** %x*\n",hwram, end1);	
		hwram_ptr = (unsigned char *)hwram;
		hwram_screen = hwram_ptr;
		hwram_ptr += HWRAM_SCREEN_SIZE;
	
		_res.MAC_loadClutData(); // scratch buffer  = "Flashback colors"
		_res.MAC_loadFontData(); // hwram taille 3352 = "Font"
		_vid.setTextPalette();

		const uint8_t* spriteData = _res._fnt;
		_res._fnt = hwram_ptr;


		DecodeBuffer buf{};
//		buf.setPixel =  _vid.MAC_setPixel;
		buf.dst_w = Video::CHAR_W*2;
		buf.dst_h = Video::CHAR_H*2;


		for (int i=0;i<106;i++)
		{
			buf.ptr = (uint8_t*)hwram_ptr;
			memset(buf.ptr, 0, buf.dst_w * buf.dst_h);
			_res.MAC_decodeImageData(spriteData, i, &buf, 0xff);

			if(i>=16 & i<26)
			{
			// on copie dans vdp1
				TEXTURE tx = TEXDEF(16, 16, position_vram);
#ifdef BPP8
				DMA_ScuMemCopy((void*)(SpriteVRAM + (tx.CGadr << 3)), (void*)buf.ptr, 16*16);
				position_vram += (256*4)>>2;
#else
				_vid.convert_8bpp_to_4bpp_inplace(buf.ptr, 16 * 16);
				DMA_ScuMemCopy((void*)(SpriteVRAM + (tx.CGadr << 3)), (void*)buf.ptr, 16*8);
				position_vram += (256*4)>>2;
#endif
				position_vram_aft_monster = position_vram;
			}
			hwram_ptr+=256;
		}
		SAT_preloadCDfiles();
		_res.load_TEXT();
#ifdef DEBUG
	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();
#endif
		_res.MAC_loadIconData(); // hwram taille 9036 = "Icons" 
#ifdef DEBUG
	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration %s : %d\n","icons", e-s);
#endif
//				SAT_preloadIcon();
#ifdef DEBUG
	_stub->initTimeStamp();
	unsigned int st = _stub->getTimeStamp();
#endif
		_res.MAC_loadPersoData();// lwram taille 213124 = "Person"
#ifdef DEBUG
	unsigned int et = _stub->getTimeStamp();
	emu_printf("--duration %s : %d\n","perso", et-st);
#endif		
//				SAT_preloadPerso();
// vbt : refaire le chargement des sons
		_res.MAC_loadSounds(); // vbt déplacé
// à redeplacer
		_res.MAC_loadCutsceneText(); // vbt déplacé
/*		break;
	}
*/
hwram = (uint8_t *)hwram_ptr;

#ifndef BYPASS_PROTECTION
	while (!handleProtectionScreen());
	if (_stub->_pi.quit) {
		return;
	}
#endif

//	if (_res.isMac()) 
	{
		_menu.displayTitleScreenMac(Menu::kMacTitleScreen_MacPlay);
		if (!_stub->_pi.quit) {
			_menu.displayTitleScreenMac(Menu::kMacTitleScreen_Presage);
		}
	}
	memset4_fast(&_vid._frontLayer[0], 0x00, _vid._layerSize);
	_stub->copyRect(0, 0, _vid._w, _vid._h, _vid._frontLayer, _vid._w);
#ifdef VIDEO_PLAYER
int i;
for (i=0;i<100;i++)
{
	current_lwram = (Uint8 *)VBT_L_START;
	emu_printf("id=%d\n",i);
	playCutscene(i);
}
#endif
	playCutscene(0x40);
	playCutscene(0x0D);

/*
	// global resources
	switch (_res._type) {
	case kResourceTypeDOS:
		_res.load("GLOBAL", Resource::OT_ICN);
		_res.load("GLOBAL", Resource::OT_SPC);
		_res.load("PERSO", Resource::OT_SPR);
		_res.load_SPR_OFF("PERSO", _res._spr1);
		break;
	case kResourceTypeMac:
		_res.MAC_loadIconData(); // vbt à faire bien avant // 19323 en HWRAM
		_res.MAC_loadPersoData(); // taille 213124 lwr
		_res.MAC_loadSounds(); //à vbt à faire bien avant 
		break;
	}
*/
	while (!_stub->_pi.quit) {
		_menu.handleTitleScreen();
#ifdef DEMO
		SAT_cleanRAM (HWRAM|LWRA1|VRAM1);

		if (_menu._selectedOption == Menu::MENU_OPTION_ITEM_DEMO) {
			_demoBin = (_demoBin + 1) % ARRAYSIZE(_demoInputs);
			const char *fn = _demoInputs[_demoBin].name;
//			emu_printf("_demoBin '%d arraysz %d\n", _demoBin, ARRAYSIZE(_demoInputs));
//			emu_printf("Loading inputs from '%s'\n", fn);
			_cheats = 0;
			_res.load_DEM(fn);
			if (_res._demLen == 0) {
				continue;
			}
			_skillLevel = kSkillNormal;
			_currentLevel = _demoInputs[_demoBin].level;
			_randSeed = 0;
			_demoRandCounter = 0;
			_demoRandRange = _demoInputsMac[_demoBin].rnd;
		} else
#endif
		{
			_cheats = kCheatOneHitKill | kCheatNoHit | kCheatOneHitKill;
			_demoBin = -1;
			_skillLevel = _menu._skill;
			_currentLevel = _menu._level;
		}
	
		_mix.stopMusic(); // vbt à remettre
		memset(_vid._frontLayer, 0, _vid._layerSize);

		if (_stub->_pi.quit) {
			break;
		}
//		if (_stub->hasWidescreen()) { // vbt à voir si on nettoie l'écran
//			_stub->clearWidescreen();
//		}
		if (_currentLevel == 7) {
			_vid.fadeOut();
			_vid.setTextPalette();
			playCutscene(0x3D);
		} else {
			_vid.setTextPalette();
			_vid.setPalette0xF();
//			_stub->setOverscanColor(0xE0);
			_stub->setOverscanColor(0x00);
			memset(_vid._backLayer, 0xE0, _vid._layerSize);
			_vid._unkPalSlot1 = 0;
			_vid._unkPalSlot2 = 0;
			_score = 0;
//			clearStateRewind();
			loadLevelData();
//emu_printf("4hwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
			resetGameState();
			_endLoop = false;
			_frameTimestamp = _stub->getTimeStamp();
			_saveTimestamp = _frameTimestamp;
			while (!_stub->_pi.quit && !_endLoop) {
				mainLoop();
#ifdef DEMO
				if (_demoBin != -1 && _inp_demPos >= _res._demLen) {
//					emu_printf("End of demo\n");
					// exit level
					_endLoop = true;
				}
#endif
			}
//			slTVOff();
			_vid._fullRefresh = true;
			memset4_fast(_vid._frontLayer,0x00,_vid._layerSize);
			_vid.updateScreen();
			_vid.SAT_cleanSprites();

			SAT_cleanRAM (HWRAM|LWRA1|VRAM1);

			// flush inputs
			_stub->_pi.dirMask = 0;
			_stub->_pi.enter = false;
			_stub->_pi.space = false;
			_stub->_pi.shift = false;
			slTVOn();
		}
	}
//	_res.free_TEXT();
	_mix.free();
	_stub->destroy();
}

void Game::resetGameState() {
	_animBuffers._states[0] = _animBuffer0State;
	_animBuffers._curPos[0] = 0xFF;
	_animBuffers._states[1] = _animBuffer1State;
	_animBuffers._curPos[1] = 0xFF;
	_animBuffers._states[2] = _animBuffer2State;
	_animBuffers._curPos[2] = 0xFF;
	_animBuffers._states[3] = _animBuffer3State;
	_animBuffers._curPos[3] = 0xFF;
	_currentRoom = _res._pgeInit[0].init_room;
	_cut._deathCutsceneId = 0xFFFF;
	_deathCutsceneCounter = 0;
	_saveStateCompleted = false;
	_loadMap = true;
	pge_resetMessages();
	_blinkingConradCounter = 0;
	_pge_processOBJ = false;
	_pge_opGunVar = 0;
	_textToDisplay = 0xFFFF;
	_pge_zoomPiegeNum = 0;
	_pge_zoomCounter = 0;
	_pge_zoomX = _pge_zoomY = 0;
}

void Game::mainLoop() {
	playCutscene();

	if (_cut._id == 0x3D) {
		showFinalScore();
		_endLoop = true;
		return;
	}
	if (_deathCutsceneCounter) {
		--_deathCutsceneCounter;
		if (_deathCutsceneCounter == 0) {
// vbt : clean front layer	
			memset(_vid._frontLayer, 0x00, _vid._layerSize);
			memset(_vid._backLayer, 0x00, _vid._layerSize);
			_stub->copyRect(0, 0, _vid._w, _vid._h, _vid._frontLayer, _vid._w);
			playCutscene(_cut._deathCutsceneId);
			if (!handleContinueAbort()) {
				playCutscene(0x41);
				_endLoop = true;
			} else {
				if (_validSaveState && loadGameState(kIngameSaveSlot)) {
					// ingame save
				} else {
//					clearStateRewind();
					loadLevelData();
// vbt : si on continue voir comment éviter de recharger les ennemis
					resetGameState();
				}
			}
			slScrAutoDisp(NBG0ON|NBG1ON|SPRON);
			slSynch();
			return;
		}
	}

	pge_getInput();
	pge_prepare();
	col_prepareRoomState();
	uint8_t oldLevel = _currentLevel;
	for (uint16_t i = 0; i < _res._pgeNum; ++i) {
		LivePGE *pge = _pge_liveTable2[i];
		if (pge) {
			_col_currentPiegeGridPosY = (pge->pos_y / 36) & ~1;
			_col_currentPiegeGridPosX = (pge->pos_x + 8) >> 4;
			pge_process(pge);
		}
	}
	if (oldLevel != _currentLevel) {
		/*if (_res._isDemo) {
			_currentLevel = oldLevel;
		}*/
//emu_printf("6hwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
		changeLevel();
//emu_printf("7hwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
		_pge_opGunVar = 0;
		_mix.playMusic(Mixer::MUSIC_TRACK + _currentLevel); // vbt : ajout sinon pas de musique, changement de niveau
		return;
	}
	if (_currentLevel == 3 && _cut._id == 50) {
		// do not draw next room when boarding taxi
		return;
	}
	if (_loadMap) {
		if (_currentRoom == 0xFF || !hasLevelRoom(_currentLevel, _pgeLive[0].room_location)) {
			_cut._id = 6;
			_deathCutsceneCounter = 1;
		} else {
			_currentRoom = _pgeLive[0].room_location;
		_mix.pauseMusic();
			loadLevelRoom();
//emu_printf("9hwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
			_loadMap = false;
 // vbt à mettre si slave reduit les plantages
			if(statdata.report.fad!=0xFFFFFF && statdata.report.fad!=0)
			{
				_mix.unpauseMusic(); // vbt : on reprend où la musique était
			}
			else
			{
				_mix.playMusic(Mixer::MUSIC_TRACK + _currentLevel); // vbt : ajout sinon pas de musique	
			}
		}
		memset4_fast(&_vid._frontLayer[51*_vid._w], 0x00,32*_vid._w);
	}
/*	if (_res.isDOS() && (_stub->_pi.dbgMask & PlayerInput::DF_AUTOZOOM) != 0) {
		pge_updateZoom();
	}*/
	prepareAnims();
	drawAnims();
	drawCurrentInventoryItem();
	drawLevelTexts();
	/*if (g_options.enable_password_menu) {
		printLevelCode();
	}*/
	if (_blinkingConradCounter != 0) {
		--_blinkingConradCounter;
	}
	_vid.updateScreen();
//	updateTiming();
	drawStoryTexts();
	if (_stub->_pi.backspace) {
		_stub->_pi.backspace = false;
		handleInventory();
	}
	if (_stub->_pi.escape) {
		_stub->_pi.escape = false;
#ifdef DEMO
		if (_demoBin != -1 || handleConfigPanel()) {
#else
if (/*_demoBin != -1*/ 0 || handleConfigPanel()) {
#endif
			_endLoop = true;
			return;
		}
	}
	
	if(_cut._stop)
	{
		if(statdata.report.fad!=0xFFFFFF && statdata.report.fad!=0)
			_mix.unpauseMusic(); // vbt : on reprend où la musique était
		else
			_mix.playMusic(Mixer::MUSIC_TRACK + _currentLevel); // vbt : ajout sinon pas de musique	
	
		_cut._stop = false;
		//audioEnabled = 1;
//		_stub->_pi.backspace = false;
//		_stub->_pi.quit = false;
	}
	inp_handleSpecialKeys();
/*	if (_autoSave && _stub->getTimeStamp() - _saveTimestamp >= kAutoSaveIntervalMs) {
		// do not save if we died or about to
		if (_pgeLive[0].life > 0 && _deathCutsceneCounter == 0) {
			saveGameState(kAutoSaveSlot);
			_saveTimestamp = _stub->getTimeStamp();
		}
	}*/
//	_vid.SAT_displayPalette();
	frame_x++;
	unsigned char c1 = frame_z/10;
	unsigned char c2 = frame_z%10;
	SPRITE user_sprite;
	user_sprite.CTRL = 0;
	user_sprite.COLR = 0;
#ifdef BPP8
	user_sprite.PMOD = CL256Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
#else
	user_sprite.COLR = 0xC0;
	user_sprite.PMOD = CL16Bnk| ECdis | 0x0800;// | ECenb | SPdis;  // pas besoin pour les sprites
#endif
	user_sprite.SRCA = 0x200+32*c1;
	user_sprite.SIZE = 0x210;
	user_sprite.XA   = 224;
	user_sprite.YA   = -220;
	slSetSprite(&user_sprite, 130<<16);	// fps
	user_sprite.SRCA = 0x200+32*c2;
	user_sprite.XA   = 240;
	slSetSprite(&user_sprite, 130<<16);	// fps	

//	_vid.SAT_displaySpritesPalette();
	slSynch();  // vbt : permet l'affichage de sprites, le principal
}
/*
void Game::updateTiming() {
	static const int frameHz = 30;
	int32_t delay = _stub->getTimeStamp() - _frameTimestamp;
	int32_t pause = (_stub->_pi.dbgMask & PlayerInput::DF_FASTMODE) ? 20 : (1000 / frameHz);
	pause -= delay;
	if (pause > 0) {
		_stub->sleep(pause);
	}
	_frameTimestamp = _stub->getTimeStamp();
}
*/
void Game::playCutscene(int id) {
//if(id>0)
//emu_printf("Cutscene::playCutscene() _id=0x%X c%p s %p\nposition_vram_aft_monster%x position_vram %x\n", id , current_lwram, save_current_lwram,position_vram_aft_monster, position_vram);
//		return;   // vbt : pour ne pas lire les videos

	if (id != -1) {
		_cut._id = id;
	}
	if (_cut._id != 0xFFFF && _cut._id != 30 && _cut._id != 31 /* && _cut._id != 22 && _cut._id != 23 && _cut._id != 24 */) {
//		_sfxPly.stop(); // vbt à voir
//		ToggleWidescreenStack tws(_stub, false);
//		_mix.stopMusic();
//		_mix.pauseMusic(); // vbt : on sauvegarde la position cdda
/*		if (_res._hasSeqData) {
			int num = 0;
			switch (_cut._id) {
			case 0x02: {
					static const uint8_t tab[] = { 1, 2, 1, 3, 3, 4, 4 };
					num = tab[_currentLevel];
				}
				break;
			case 0x05: {
					static const uint8_t tab[] = { 1, 2, 3, 5, 5, 4, 4 };
					num = tab[_currentLevel];
				}
				break;
			case 0x0A: {
					static const uint8_t tab[] = { 1, 2, 2, 2, 2, 2, 2 };
					num = tab[_currentLevel];
				}
				break;
			case 0x10: {
					static const uint8_t tab[] = { 1, 1, 1, 2, 2, 3, 3 };
					num = tab[_currentLevel];
				}
				break;
			case 0x3C: {
					static const uint8_t tab[] = { 1, 1, 1, 1, 1, 2, 2 };
					num = tab[_currentLevel];
				}
				break;
			case 0x40:
				return;
			case 0x4A:
				return;
			}
			if (SeqPlayer::_namesTable[_cut._id]) {
			        char name[16];
			        snprintf(name, sizeof(name), "%s.SEQ", SeqPlayer::_namesTable[_cut._id]);
				char *p = strchr(name, '0');
				if (p) {
					*p += num;
				}
			        if (playCutsceneSeq(name)) {
					if (_cut._id == 0x3D) {
						playCutsceneSeq("CREDITS.SEQ");
						_cut._interrupted = false;
					} else {
						_cut._id = 0xFFFF;
					}
					_mix.stopMusic();
					return;
				}
			}
		}
*/
		if (_cut._id == 0x3D) {
//			playCutsceneSeq("CREDITS.SEQ");
			_cut._interrupted = false;
		}
		/*if (_res.isAmiga()) {
			const int num = Cutscene::_musicTableAmiga[_cut._id * 2];
			if (num != 0xFF) {
				const int bpm = Cutscene::_musicTableAmiga[_cut._id * 2 + 1];
				_mix.playMusic(num, bpm);
			}
		} else
			{
			const int num = Cutscene::_musicTableDOS[_cut._id];
			if (num != 0xFF) {
				_mix.playMusic(num);
			}
		}*/
//		emu_printf("_cut._id %d _music %d\n",_cut._id,_cut._musicTableDOS[_cut._id]);
		_cut.play();
		if (id == 0xD && !_cut._interrupted) {
//			if (!_res.isAmiga()) 
			{
				_cut._id = 0x4A; // second part of the introduction cutscene
				_mix.pauseMusic(); // vbt : on sauvegarde la position cdda			
				_cut.play();
			}
		}
		
#if 0 
		if (/*_res.isMac() &&*/ !(id == 0x48 || id == 0x49)) { // continue or score screens
			// restore palette entries modified by the cutscene player (0xC and 0xD)
			Color palette[32];
			_res.MAC_copyClut16(palette, 0, 0x37);
			_res.MAC_copyClut16(palette, 1, 0x38);
//			for (int i = 0; i < 32; ++i) {
			for (int i = 0; i < 14; ++i) { // vbt on n'écrase pas les 2 couleurs de priorité
				_stub->setPaletteEntry(0xC0 + i, &palette[i]);
			}
			_stub->setPaletteEntry(64, &palette[14]);
			_stub->setPaletteEntry(65, &palette[15]);

			for (int i = 16; i < 30; ++i) { // vbt on n'écrase pas les 2 couleurs de priorité
				_stub->setPaletteEntry(0xC0 + i, &palette[i]);
			}
			_stub->setPaletteEntry(66, &palette[31]);
			_stub->setPaletteEntry(67, &palette[32]);
		}
#endif
		if (_cut._id == 0x3D) {
			_mix.playMusic(Mixer::MUSIC_TRACK + 9);
			_cut.playCredits();
		}
		_mix.stopMusic();
	
		frame_y = frame_x = 0;
		frame_z = 30;
	}
	else
	{  // vbt pour les niveaux sans video
		if(_mix._musicTrack==2)
			_mix.stopMusic();
		slScrAutoDisp(NBG0ON|NBG1ON|SPRON);
	}
}

void Game::inp_handleSpecialKeys() {
	if (_stub->_pi.dbgMask & PlayerInput::DF_SETLIFE) {
		_pgeLive[0].life = 0x7FFF;
	}
	if (_stub->_pi.load) {
		loadGameState(_stateSlot);
		_stub->_pi.load = false;
	}
	if (_stub->_pi.save) {
		saveGameState(_stateSlot);
		_stub->_pi.save = false;
	}
	if (_stub->_pi.stateSlot != 0) {
		int8_t slot = _stateSlot + _stub->_pi.stateSlot;
		if (slot >= 1 && slot < 100) {
			_stateSlot = slot;
//			debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
		}
		_stub->_pi.stateSlot = 0;
	}
/*	
	if (_stub->_pi.rewind) {
		if (_rewindLen != 0) {
			loadStateRewind();
		} else {
			debug(DBG_INFO, "Rewind buffer is empty");
		}
		_stub->_pi.rewind = false;
	}
*/	
}

void Game::drawCurrentInventoryItem() {
	uint16_t src = _pgeLive[0].current_inventory_PGE;
	if (src != 0xFF) {
		_currentIcon = _res._pgeInit[src].icon_num;
		drawIcon(_currentIcon, 232, 8, 0xA);
	}
}

void Game::showFinalScore() {
//	if (_stub->hasWidescreen()) {   // vbt à voir si on remet
//		_stub->clearWidescreen();
//	}
	playCutscene(0x49);
	char buf[50];
	snprintf(buf, sizeof(buf), "SCORE %08u", _score);
	_vid.drawString(buf, (Video::GAMESCREEN_W - strlen(buf) * Video::CHAR_W) / 2, 40, 0xE5);
/*
	const char *str = _menu.getLevelPassword(7, _skillLevel);
	_vid.drawString(str, (Video::GAMESCREEN_W - strlen(str) * Video::CHAR_W) / 2, 16, 0xE7);
*/	
	while (!_stub->_pi.quit) {
		_stub->copyRect(0, 0, _vid._w, _vid._h, _vid._frontLayer, _vid._w);
//		_stub->updateScreen(0);
		_stub->processEvents();
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			break;
		}
		_stub->sleep(100);
	}
}

bool Game::handleConfigPanel() {
	static const int x = 7;
	static const int y = 10;
	static const int w = 17;
	static const int h = 12;

	_vid._charShadowColor = 0xE2;
	_vid._charFrontColor = 0xEE;
	_vid._charTransparentColor = 0xFF;

	// the panel background is drawn using special characters from FB_TXT.FNT
//	static const bool kUseDefaultFont = true;

/*	switch (_res._type) {
	case kResourceTypeAmiga:
		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j) {
				_vid.fillRect(Video::CHAR_W * (x + j), Video::CHAR_H * (y + i), Video::CHAR_W, Video::CHAR_H, 0xE2);
			}
		}
		break;
	case kResourceTypeDOS:
	case kResourceTypePC98:
		// top-left rounded corner
		_vid.DOS_drawChar(0x81, y, x, kUseDefaultFont);
		// top-right rounded corner
		_vid.DOS_drawChar(0x82, y, x + w, kUseDefaultFont);
		// bottom-left rounded corner
		_vid.DOS_drawChar(0x83, y + h, x, kUseDefaultFont);
		// bottom-right rounded corner
		_vid.DOS_drawChar(0x84, y + h, x + w, kUseDefaultFont);
		// horizontal lines
		for (int i = 1; i < w; ++i) {
			_vid.DOS_drawChar(0x85, y, x + i, kUseDefaultFont);
			_vid.DOS_drawChar(0x88, y + h, x + i, kUseDefaultFont);
		}
		for (int j = 1; j < h; ++j) {
			_vid._charTransparentColor = 0xFF;
			// left vertical line
			_vid.DOS_drawChar(0x86, y + j, x, kUseDefaultFont);
			// right vertical line
			_vid.DOS_drawChar(0x87, y + j, x + w, kUseDefaultFont);
			_vid._charTransparentColor = 0xE2;
			for (int i = 1; i < w; ++i) {
				_vid.DOS_drawChar(0x20, y + j, x + i, kUseDefaultFont);
			}
		}
		break;
	case kResourceTypeMac:*/
		// top-left rounded corner
		_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * x,       Video::CHAR_H * y,       _res._fnt, _vid._charFrontColor, 0x81);
		// top-right rounded corner
		_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * (x + w), Video::CHAR_H * y,       _res._fnt, _vid._charFrontColor, 0x82);
		// bottom-left rounded corner
		_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * x,       Video::CHAR_H * (y + h), _res._fnt, _vid._charFrontColor, 0x83);
		// bottom-right rounded corner
		_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * (x + w), Video::CHAR_H * (y + h), _res._fnt, _vid._charFrontColor, 0x84);
		// horizontal lines
		for (int i = 1; i < w; ++i) {
			_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * (x + i), Video::CHAR_H * y,       _res._fnt, _vid._charFrontColor, 0x85);
			_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * (x + i), Video::CHAR_H * (y + h), _res._fnt, _vid._charFrontColor, 0x88);
		}
		// vertical lines
		for (int i = 1; i < h; ++i) {
			_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * x,       Video::CHAR_H * (y + i), _res._fnt, _vid._charFrontColor, 0x86);
			_vid.MAC_drawStringChar(_vid._frontLayer, _vid._w, Video::CHAR_W * (x + w), Video::CHAR_H * (y + i), _res._fnt, _vid._charFrontColor, 0x87);
			for (int j = 1; j < w; ++j) {
				_vid.fillRect(Video::CHAR_W * (x + j), Video::CHAR_H * (y + i), Video::CHAR_W, Video::CHAR_H, 0xE2);
			}
		}
/*		break;
	}*/
	_stub->_pi.quit = false;
	_stub->_pi.escape = false;

	_menu._charVar3 = 0xE4;
	_menu._charVar4 = 0xE5;
	_menu._charVar1 = 0xE2;
	_menu._charVar2 = 0xEE;

	enum { MENU_ITEM_ABORT = 1, MENU_ITEM_LOAD = 2, MENU_ITEM_SAVE = 3 };
	uint8_t colors[] = { 2, 3, 3, 3 };
	int current = 0;

	_vid.fillRect(Video::CHAR_W * (x + 1), Video::CHAR_H * (y + 10), Video::CHAR_W * (w - 2), Video::CHAR_H, 0xE2);

	while (!_stub->_pi.quit) {
		_menu.drawString(_res.getMenuString(LocaleData::LI_18_RESUME_GAME), y + 2, 9, colors[0]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_19_ABORT_GAME), y + 4, 9, colors[1]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_20_LOAD_GAME), y + 6, 9, colors[2]);
		_menu.drawString(_res.getMenuString(LocaleData::LI_21_SAVE_GAME), y + 8, 9, colors[3]);
		_vid.fillRect(Video::CHAR_W * (x + 1), Video::CHAR_H * (y + 10), Video::CHAR_W * (w - 2), Video::CHAR_H, 0xE2);
		char buf[32];
		snprintf(buf, sizeof(buf), "%s < %02d >", _res.getMenuString(LocaleData::LI_22_SAVE_SLOT), _stateSlot);
		_menu.drawString(buf, y + 10, 9, 1);
		_stub->copyRect(112, 160, 288, 208, _vid._frontLayer, _vid._w);
		_stub->updateScreen(0);
		_stub->sleep(80);
#ifdef DEMO
		inp_update();
#endif

		int prev = current;
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			current = (current + 3) % 4;
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			current = (current + 1) % 4;
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			--_stateSlot;
			if (_stateSlot < 1) {
				_stateSlot = 1;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			++_stateSlot;
			if (_stateSlot > 3) {
				_stateSlot = 3;
			}
		}
		if (prev != current) {
			SWAP(colors[prev], colors[current]);
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			switch (current) {
			case MENU_ITEM_LOAD:
				_stub->_pi.load = true;
				break;
			case MENU_ITEM_SAVE:
				_stub->_pi.save = true;
				break;
			}
			break;
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
	}
	_stub->_pi.quit = false;

	if (current != MENU_ITEM_ABORT)
	{
		memset4_fast(&_vid._frontLayer[160*512],0x00,_vid._w*208);
		_stub->copyRect(112, 160, 288, 208, _vid._frontLayer, _vid._w);
		_stub->updateScreen(0);
	}
	
	frame_y = frame_x = 0;
	frame_z = 30;
	
	return (current == MENU_ITEM_ABORT);
}

bool Game::handleContinueAbort() {
		
	playCutscene(0x48);
	
	char textBuf[50];
	int timeout = 100;
	int current_color = 0;
	uint8_t colors[] = { 0xE4, 0xE5 };
	uint8_t color_inc = 0xFF;
	Color col;
	_stub->getPaletteEntry(0xE4, &col);

	while (timeout >= 0 && !_stub->_pi.quit) {
		const char *str;
		str = _res.getMenuString(LocaleData::LI_01_CONTINUE_OR_ABORT);
		_vid.drawString(str, (264 - strlen(str) * 8) / 2, 76, 0xE3);
		str = _res.getMenuString(LocaleData::LI_02_TIME);
		
		sprintf(textBuf, "%s: %d", str, timeout / 10);
		memset(&_vid._frontLayer[(127<<1)*512],0x00,14*512);
		_vid.drawString(textBuf, 61, 127, 0xE3);
		str = _res.getMenuString(LocaleData::LI_03_CONTINUE);
		_vid.drawString(str, 61, 104, colors[0]);
		str = _res.getMenuString(LocaleData::LI_04_ABORT);
		_vid.drawString(str, 166, 104, colors[1]);
		sprintf(textBuf, "SCORE: %lu", _score);
		_vid.drawString(textBuf, 64, 153, 0xE3);

//emu_printf("slsynch Game::handleContinueAbort()\n");
//		slSynch();

//		if (_res.isMac()) {

			if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				if (current_color > 0) {
					SWAP(colors[current_color], colors[current_color - 1]);
					--current_color;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				if (current_color < 1) {
					SWAP(colors[current_color], colors[current_color + 1]);
					++current_color;
				}
			}
/*		}
		else
		{
			if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
				if (current_color > 0) {
					SWAP(colors[current_color], colors[current_color - 1]);
					--current_color;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				if (current_color < 1) {
					SWAP(colors[current_color], colors[current_color + 1]);
					++current_color;
				}
			}
		}
*/
		if (_stub->_pi.enter) {
// vbt : corrige la ram quand on continue
			SAT_cleanRAM (LWRAM);
// vbt : corrige la ram quand on continue
			_stub->_pi.enter = false;
			memset4_fast(_vid._frontLayer,0x00,_vid._layerSize);
			_stub->copyRect(0, 0, _vid._w, _vid._h, _vid._frontLayer, _vid._w);
			_vid.SAT_cleanSprites();
			return (current_color == 0);
		}
		_stub->copyRect(0, 0, _vid._w, 400, _vid._frontLayer, _vid._w);

		static const int COLOR_STEP = 8;
		static const int COLOR_MIN = 16;
		static const int COLOR_MAX = 256 - 16;
		if (col.b >= COLOR_MAX) {
			color_inc = 0;
		} else if (col.b < COLOR_MIN) {
			color_inc = 0xFF;
		}
		if (color_inc == 0xFF) {
			col.b += COLOR_STEP;
			col.g += COLOR_STEP;
		} else {
			col.b -= COLOR_STEP;
			col.g -= COLOR_STEP;
		}
		_stub->setPaletteEntry(0xE4, &col);
		_stub->processEvents();
		_stub->sleep(100);
		--timeout;
	}
	memset4_fast(_vid._frontLayer,0x00,_vid._layerSize);
	_stub->copyRect(0, 0, _vid._w, _vid._h, _vid._frontLayer, _vid._w);
	return false;
}
/*
void Game::printLevelCode() {
	if (_printLevelCodeCounter != 0) {
		--_printLevelCodeCounter;
		if (_printLevelCodeCounter != 0) {
			char buf[32];
			snprintf(buf, sizeof(buf), "CODE: %s", _menu.getLevelPassword(_currentLevel, _skillLevel));
			_vid.drawString(buf, (Video::GAMESCREEN_W - strlen(buf) * Video::CHAR_W) / 2, 16, 0xE7);
		}
	}
}
*/
void Game::printSaveStateCompleted() {
	if (_saveStateCompleted) {
		const char *str = _res.getMenuString(LocaleData::LI_05_COMPLETED);
		_vid.drawString(str, (176 - strlen(str) * Video::CHAR_W) / 2, 34, 0xE6);
	}
}
int hasLevelText = false;
int previousText_num = -1;
void Game::drawLevelTexts() {
	LivePGE *pge = &_pgeLive[0];
	int8_t obj = col_findCurrentCollidingObject(pge, 3, 0xFF, 0xFF, &pge);
	if (obj == 0) {
		obj = col_findCurrentCollidingObject(pge, 0xFF, 5, 9, &pge);
	}
	
	if(hasLevelText)
	{
//		emu_printf("_textToDisplay %x obj %d num %d\n",_textToDisplay,obj,pge->init_PGE->text_num);
		if(previousText_num!=pge->init_PGE->text_num)
		{
//			emu_printf("erase %x obj %d\n",_textToDisplay,obj);
			memset4_fast(&_vid._frontLayer[51 << 9], 0x00,32 << 9);
			_stub->copyRect(0, 51, _vid._w, 32, _vid._frontLayer, _vid._w);
		}
		hasLevelText = false;
	}

	if (obj > 0) {
		hasLevelText = true;		
//		_printLevelCodeCounter = 0;
		if (_textToDisplay == 0xFFFF) {
			uint8_t icon_num = obj - 1;
			drawIcon(icon_num, 80, 8, 0xA);
			uint8_t txt_num = pge->init_PGE->text_num;
			const uint8_t *str = _res.getTextString(_currentLevel, txt_num);
//			char toto [256];
//			memcpy(&toto[2],str,*str);
//	emu_printf("cut id %d drawLevelTexts %s\n",_cut._id,toto);
//			memset4_fast(&_vid._frontLayer[51*_vid._w],0x00,16*_vid._w);	
			if (_cut._id == 0xFFFF) // ou !=34 ou remonter avec texttodisplay
				drawString(str, 176, 26, 0xE6, true);

			if (icon_num == 2) {
				printSaveStateCompleted();
//				return;
			}
		} 
//		else 
		{
//			_currentInventoryIconNum = obj - 1;
			_stub->copyRect(0, 51, _vid._w, 32, _vid._frontLayer, _vid._w);
		}
//		emu_printf("redraw %x obj %d %s\n",_textToDisplay,obj);
	}
	previousText_num = pge->init_PGE->text_num;
	_saveStateCompleted = false;
}

static int getLineLength(const uint8_t *str) {
	int len = 0;
	while (*str && *str != 0xB && *str != 0xA) {
		++str;
		++len;
	}
	return len;
}

void Game::drawStoryTexts() {
	if (_textToDisplay != 0xFFFF) {
		uint8_t textColor = 0xE8;
		const uint8_t *str = _res.getGameString(_textToDisplay);
		int textSpeechSegment = 0;
		int textSegmentsCount = 0;
		int yPos = 0;
		while (!_stub->_pi.quit) {
			memset(_vid._frontLayer, 0x00, 512*224);
//			drawIcon(_currentInventoryIconNum, 80, 8, 0xA);
			yPos = 26;
//			if (_res._type == kResourceTypeMac) {
				if (textSegmentsCount == 0) {
					textSegmentsCount = *str++;
				}
				int len = *str++;
				if (*str == '@' /* Floppy version */ || *str == '~' /* CD version */) {
					switch (str[1]) {
					case '1':
						textColor = 0xE9;
						break;
					case '2':
						textColor = 0xEB;
						break;
					default:
//						warning("Unhandled MAC text color code 0x%x", str[1]);
						break;
					}
					str += 2;
					len -= 2;
				}
				for (; len > 0; yPos += 8) {
					const uint8_t *next = (const uint8_t *)memchr(str, 0x7C, len);
					if (!next) {
						_vid.drawStringLen((const char *)str, len, (176 - len * Video::CHAR_W) / 2, yPos, textColor);
						// point 'str' to beginning of next text segment
						str += len;
						break;
					}
					const int lineLength = next - str;
					_vid.drawStringLen((const char *)str, lineLength, (176 - lineLength * Video::CHAR_W) / 2, yPos, textColor);
					str = next + 1;
					len -= lineLength + 1;
				}
/*			} else {
				if (*str == 0xFF) {
					if (_res._lang == LANG_JP) {
						switch (str[1]) {
						case 0:
							textColor = 0xE9;
							break;
						case 1:
							textColor = 0xEB;
							break;
						default:
							warning("Unhandled JP text color code 0x%x", str[1]);
							break;
						}
						str += 2;
					} else {
						textColor = str[1];
						// str[2] is an unused color (possibly the shadow)
						str += 3;
					}
				}
				while (1) {
					const int len = getLineLength(str);
					str = (const uint8_t *)_vid.drawString((const char *)str, (176 - len * Video::CHAR_W) / 2, yPos, textColor);
					if (*str == 0 || *str == 0xB) {
						break;
					}
					++str;
					yPos += 8;
				}
			}*/
			uint8_t *voiceSegmentData = 0;
			uint32_t voiceSegmentLen = 0;
			uint32_t next = 0;
//			_textToDisplay=0;
			_mix.pauseMusic();
			//_mix.stopMusic();
			pcm_sample_stop(PCM_VOICE);
			volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(PCM_VOICE);
			volatile scsp_dbg_reg_t *dbg_reg = (scsp_dbg_reg_t *)get_scsp_dbg_reg();
			dbg_reg->mslc= PCM_VOICE;
			_res.load_VCE(_textToDisplay, textSpeechSegment++, &voiceSegmentData, &voiceSegmentLen);
//			emu_printf("load_VCE XXXX %d nb seg %d size %d current segment %d\n",_textToDisplay,textSegmentsCount,voiceSegmentLen,textSpeechSegment-1);
//			if (voiceSegmentData) {
			if (voiceSegmentLen) 
			{
				uint32_t address = (uint32_t)soundAddr;
//				emu_printf("load_VCE num %d len %d segment %d addr %x\n",_textToDisplay,voiceSegmentLen,textSpeechSegment,address);
				pcm_sample_t pcm = {.addr = address, .vol = (Mixer::MAX_VOLUME>>1)-1, .bit = pcm_sample_8bit};
				pcm_prepare_sample(&pcm, PCM_VOICE, voiceSegmentLen);
	//			pcm_sample_set_samplerate(&pcm, sfx->freq);
				pcm_sample_set_samplerate(PCM_VOICE, 11035);
				pcm_sample_set_loop(PCM_VOICE, pcm_sample_loop_no_loop);
				pcm_sample_start(PCM_VOICE);
//				_mix.play(voiceSegmentData, voiceSegmentLen, 32000, Mixer::MAX_VOLUME);  // vbt ࠲emettre
			}
/*
char toto[100];
sprintf(toto,"sta ca%x sa%x lsa%d lea%04x", dbg_reg->ca,slot->sa,slot->lsa,slot->lea);
_vid.drawString(toto, 1, 78, 0xE7);
			emu_printf("start play slot %d ca %04x pcm size %d sa %d lsa %04x lea %04x\n",PCM_VOICE, dbg_reg->ca,voiceSegmentLen,slot->sa,slot->lsa,slot->lea);
*/
//emu_printf("draw story text\n");	
			_stub->copyRect(0, 51, _vid._w, yPos*4, _vid._frontLayer, _vid._w);

			while (!_stub->_pi.backspace && !_stub->_pi.quit) {
				/*if (voiceSegmentData && !_mix.isPlaying(voiceSegmentData)) {
					break;
				}*/
//sprintf(toto,"%06d %06d %d", (dbg_reg->ca+1)*4096,voiceSegmentLen-1,((dbg_reg->ca+1)*4096>=voiceSegmentLen-1));
//_vid.drawString(toto, 1, 70, 0xE7);
//emu_printf("(dbg_reg->ca+1)*4096 %d voiceSegmentLen %d comp %d\n",(dbg_reg->ca+1)*4096,voiceSegmentLen,((dbg_reg->ca+1)*4096>=voiceSegmentLen-1));
				if((dbg_reg->ca+1)*4096>=voiceSegmentLen-1
				|| (next>=voiceSegmentLen-1 && dbg_reg->ca==0))
				{
/*
sprintf(toto,"end ca%x sa%x lsa%d lea%04x", dbg_reg->ca,slot->sa,slot->lsa,slot->lea);
_vid.drawString(toto, 1, 88, 0xE7);
				emu_printf("end play slot %d ca %04x sa %d lsa %04x lea %04x\n",PCM_VOICE, dbg_reg->ca,slot->sa,slot->lsa,slot->lea);
				_stub->sleep(2000);
*/
					pcm_sample_stop(PCM_VOICE);
					break;
				}
				next=(dbg_reg->ca+2)*4096;
#ifdef DEMO
				inp_update();
#endif
				_stub->sleep(80);
			}

			if (voiceSegmentLen) {
				pcm_sample_stop(PCM_VOICE);
//				_mix.stopAll();
//				sat_free(voiceSegmentData);
			}
			_stub->_pi.quit = false;
			_stub->_pi.backspace = false;
//			if (_res._type == kResourceTypeMac) {
				if (textSpeechSegment >= textSegmentsCount) {
					break;
				}
/*			} else {
				if (*str == 0) {
					break;
				}
				++str;
			}*/
		}
//emu_printf("clean storytext\n");			
		memset4_fast(&_vid._frontLayer[51*_vid._w], 0x00, _vid._w*yPos*4);    // vbt : inutile pour la fin d'un message de plus d'une ligne
		_stub->copyRect(0, 51, _vid._w, yPos*4, _vid._frontLayer, _vid._w);
		_textToDisplay = 0xFFFF;
		_stub->_pi.backspace = false;
		_stub->_pi.quit = false;

		_mix.unpauseMusic();
		frame_y = frame_x = 0;
		frame_z = 30;
	}
}

void Game::drawString(const uint8_t *p, int x, int y, uint8_t color, bool hcenter) {
	const char *str = (const char *)p;
	int len = 0;
//	if (_res._type == kResourceTypeMac) {
		len = *p;
		++str;
/*	} else {
		len = strlen(str);
	}*/
	if (hcenter) {
		x = (x - len * Video::CHAR_W) / 2;
	}
	_vid.drawStringLen(str, len, x, y, color);
}

void Game::prepareAnims() {
	if (_currentRoom < 0x40) {
		int8_t pge_room;
		LivePGE *pge = _pge_liveTable1[_currentRoom];
		while (pge) {
			prepareAnimsHelper(pge, 0, 0);
			pge = pge->next_PGE_in_room;
		}
		pge_room = _res._ctData[CT_UP_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if ((pge->init_PGE->object_type != 10 && pge->pos_y > 176) || (pge->init_PGE->object_type == 10 && pge->pos_y > 216)) {
					prepareAnimsHelper(pge, 0, -216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_DOWN_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_y < 48) {
					prepareAnimsHelper(pge, 0, 216);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_LEFT_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_x > 224) {
					prepareAnimsHelper(pge, -256, 0);
				}
				pge = pge->next_PGE_in_room;
			}
		}
		pge_room = _res._ctData[CT_RIGHT_ROOM + _currentRoom];
		if (pge_room >= 0 && pge_room < 0x40) {
			pge = _pge_liveTable1[pge_room];
			while (pge) {
				if (pge->pos_x <= 32) {
					prepareAnimsHelper(pge, 256, 0);
				}
				pge = pge->next_PGE_in_room;
			}
		}
	}
}

void Game::prepareAnimsHelper(LivePGE *pge, int16_t dx, int16_t dy) {
//	debug(DBG_GAME, "Game::prepareAnimsHelper() dx=0x%X dy=0x%X pge_num=%ld pge->flags=0x%X pge->anim_number=0x%X", dx, dy, pge - &_pgeLive[0], pge->flags, pge->anim_number);
	if (!(pge->flags & 8)) {
		if (pge->index != 0 && loadMonsterSprites(pge) == 0) {
			return;
		}
		const uint8_t *dataPtr = 0;
		int8_t dw = 0, dh = 0;
		/*switch (_res._type) {
		case kResourceTypeDOS:
			assert(pge->anim_number < 1287);
//			dataPtr = _res._sprData[pge->anim_number];
			if (dataPtr == 0) {
				return;
			}
			dw = (int8_t)dataPtr[0];
			dh = (int8_t)dataPtr[1];
			break;
		case kResourceTypeMac:
			break;
		}*/
		uint8_t w = 0, h = 0;
/*		switch (_res._type) {
		case kResourceTypeDOS:
			w = dataPtr[2];
			h = dataPtr[3];
			dataPtr += 4;
			break;
		case kResourceTypeMac:
			break;
		}*/
		int16_t ypos = dy + pge->pos_y - dh + 2;
		int16_t xpos = dx + pge->pos_x - dw;
		if (pge->flags & 2) {
			xpos = dw + dx + pge->pos_x;
			uint8_t _cl = w;
			if (_cl & 0x40) {
				_cl = h;
			} else {
				_cl &= 0x3F;
			}
			xpos -= _cl;
		}
		if (xpos <= -32 || xpos >= 256 || ypos < -48 || ypos >= 224) {
			return;
		}
		xpos += 8;
		if (pge == &_pgeLive[0]) {
			_animBuffers.addState(1, xpos, ypos, dataPtr, pge, w, h);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge, w, h);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge, w, h);
		}
	} else {
		const uint8_t *dataPtr = 0;
/*		switch (_res._type) {
		case kResourceTypeDOS:
			assert(pge->anim_number < _res._numSpc);
			dataPtr = _res._spc + READ_BE_UINT16(_res._spc + pge->anim_number * 2);
			break;
		case kResourceTypeMac:
			break;
		}*/
		const int16_t xpos = dx + pge->pos_x + 8;
		const int16_t ypos = dy + pge->pos_y + 2;
		if (pge->init_PGE->object_type == 11) {
			_animBuffers.addState(3, xpos, ypos, dataPtr, pge);
		} else if (pge->flags & 0x10) {
			_animBuffers.addState(2, xpos, ypos, dataPtr, pge);
		} else {
			_animBuffers.addState(0, xpos, ypos, dataPtr, pge);
		}
		dataPtr = NULL; // vbt : ajout
	}
}

void Game::drawAnims() {
	drawAnimBuffer(2, _animBuffer2State);
	drawAnimBuffer(1, _animBuffer1State);
	drawAnimBuffer(0, _animBuffer0State);
//	_eraseBackground = true;
	drawAnimBuffer(3, _animBuffer3State);
}

void Game::drawAnimBuffer(uint8_t stateNum, AnimBufferState *state) {
//	emu_printf("Game::drawAnimBuffer() state=%d\n", stateNum);
//	assert(stateNum < 4);
	if(stateNum >= 4)
		return;
	_animBuffers._states[stateNum] = state;
	uint8_t lastPos = _animBuffers._curPos[stateNum];
	if (lastPos != 0xFF) {
		uint8_t numAnims = lastPos + 1;
		state += lastPos;
		_animBuffers._curPos[stateNum] = 0xFF;
		do {
			LivePGE *pge = state->pge;
			if (!(pge->flags & 8)) {
				if (stateNum == 1 && (_blinkingConradCounter & 1)) {
					break;
				}
			}/* else {
				drawPiege(state);
			}*/
			drawPiege(state);
			--state;
		} while (--numAnims != 0);
	}
}

void Game::drawPiege(AnimBufferState *state) {
	LivePGE *pge = state->pge;
/*	switch (_res._type) {
//	case kResourceTypeDOS:
//		drawObject(state->dataPtr, state->x, state->y, pge->flags);
//		break;
	case kResourceTypeMac:*/
		if (pge->flags & 8) {
			_vid.MAC_drawSprite(state->x, state->y, _res._spc, pge->anim_number, 0, (pge->flags & 2) != 0);
		} else if (pge->index == 0) {
			if (pge->anim_number == 0x386) {
//				break;
				return;
			}
			const int frame = _res.MAC_getPersoFrame(pge->anim_number);
			_vid.MAC_drawSprite(state->x, state->y, _res._perso, frame, pge->anim_number, (pge->flags & 2) != 0);
		} else {
//emu_printf("MAC_drawSprite 3 monster\n");
// vbt : gerer le cas du sprite pas en vram !!!!
			const int frame = _res.MAC_getMonsterFrame(pge->anim_number);
			_vid.MAC_drawSprite(state->x, state->y, _res._monster, frame, pge->anim_number, (pge->flags & 2) != 0);
		}
/*		break;
	}*/
}
/*
void Game::drawObject(const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags) {
	assert(dataPtr[0] < 0x4A);
	uint8_t slot = _res._rp[dataPtr[0]];
	uint8_t *data = _res.findBankData(slot);
	if (data == 0) {
		data = _res.loadBankData(slot);
	}
	int16_t posy = y - (int8_t)dataPtr[2];
	int16_t posx = x;
	if (flags & 2) {
		posx += (int8_t)dataPtr[1];
	} else {
		posx -= (int8_t)dataPtr[1];
	}
	int count = 0;
	switch (_res._type) {
	case kResourceTypeDOS:
		count = dataPtr[5];
		dataPtr += 6;
		break;
	case kResourceTypeMac:
		assert(0); // different graphics format
		break;
	}
	for (int i = 0; i < count; ++i) {
		drawObjectFrame(data, dataPtr, posx, posy, flags);
		dataPtr += 4;
	}
}

void Game::drawObjectFrame(const uint8_t *bankDataPtr, const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags) {
	debug(DBG_GAME, "Game::drawObjectFrame(%p, %d, %d, 0x%X)", dataPtr, x, y, flags);
	const uint8_t *src = bankDataPtr + dataPtr[0] * 32;

	int16_t sprite_y = y + dataPtr[2];
	int16_t sprite_x;
	if (flags & 2) {
		sprite_x = x - dataPtr[1] - (((dataPtr[3] & 0xC) + 4) * 2);
	} else {
		sprite_x = x + dataPtr[1];
	}

	uint8_t sprite_flags = dataPtr[3];
	if (flags & 2) {
		sprite_flags ^= 0x10;
	}

	const uint8_t sprite_h = (((sprite_flags >> 0) & 3) + 1) * 8;
	const uint8_t sprite_w = (((sprite_flags >> 2) & 3) + 1) * 8;

	switch (_res._type) {
	case kResourceTypeDOS:
//		_vid.PC_decodeSpc(src, sprite_w, sprite_h, _res._scratchBuffer);   // vbt à remettre
		break;
	case kResourceTypeMac:
		assert(0); // different graphics format
		break;
	}

	src = _res._scratchBuffer;
	bool sprite_mirror_x = false;
	int16_t sprite_clipped_w;
	if (sprite_x >= 0) {
		sprite_clipped_w = sprite_x + sprite_w;
		if (sprite_clipped_w < Video::GAMESCREEN_W) {
			sprite_clipped_w = sprite_w;
		} else {
			sprite_clipped_w = Video::GAMESCREEN_W - sprite_x;
			if (sprite_flags & 0x10) {
				sprite_mirror_x = true;
				src += sprite_w - 1;
			}
		}
	} else {
		sprite_clipped_w = sprite_x + sprite_w;
		if (!(sprite_flags & 0x10)) {
			src -= sprite_x;
			sprite_x = 0;
		} else {
			sprite_mirror_x = true;
			src += sprite_x + sprite_w - 1;
			sprite_x = 0;
		}
	}
	if (sprite_clipped_w <= 0) {
		return;
	}

	int16_t sprite_clipped_h;
	if (sprite_y >= 0) {
		sprite_clipped_h = Video::GAMESCREEN_H - sprite_h;
		if (sprite_y < sprite_clipped_h) {
			sprite_clipped_h = sprite_h;
		} else {
			sprite_clipped_h = Video::GAMESCREEN_H - sprite_y;
		}
	} else {
		sprite_clipped_h = sprite_h + sprite_y;
		src -= sprite_w * sprite_y;
		sprite_y = 0;
	}
	if (sprite_clipped_h <= 0) {
		return;
	}

	if (!sprite_mirror_x && (sprite_flags & 0x10)) {
		src += sprite_w - 1;
	}

	const uint32_t dst_offset = Video::GAMESCREEN_W * sprite_y + sprite_x;
	const uint8_t sprite_col_mask = (flags & 0x60) >> 1;

	if (_eraseBackground) {
		if (!(sprite_flags & 0x10)) {
			_vid.drawSpriteSub1(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub2(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	} else {
		if (!(sprite_flags & 0x10)) {
			_vid.drawSpriteSub3(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub4(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	}
//	_vid.markBlockAsDirty(sprite_x, sprite_y, sprite_clipped_w, sprite_clipped_h, _vid._layerScale);
}

void Game::drawCharacter(const uint8_t *dataPtr, int16_t pos_x, int16_t pos_y, uint8_t a, uint8_t b, uint8_t flags) {
	debug(DBG_GAME, "Game::drawCharacter(%p, %d, %d, 0x%X, 0x%X, 0x%X)", dataPtr, pos_x, pos_y, a, b, flags);
	bool sprite_mirror_y = false;
	if (b & 0x40) {
		b &= ~0x40;
		SWAP(a, b);
		sprite_mirror_y = true;
	}
	uint16_t sprite_h = a;
	uint16_t sprite_w = b;

	const uint8_t *src = dataPtr;
	bool sprite_mirror_x = false;

	int16_t sprite_clipped_w;
	if (pos_x >= 0) {
		if (pos_x + sprite_w < 256) {
			sprite_clipped_w = sprite_w;
		} else {
			sprite_clipped_w = 256 - pos_x;
			if (flags & 2) {
				sprite_mirror_x = true;
				if (sprite_mirror_y) {
					src += (sprite_w - 1) * sprite_h;
				} else {
					src += sprite_w - 1;
				}
			}
		}
	} else {
		sprite_clipped_w = pos_x + sprite_w;
		if (!(flags & 2)) {
			if (sprite_mirror_y) {
				src -= sprite_h * pos_x;
				pos_x = 0;
			} else {
				src -= pos_x;
				pos_x = 0;
			}
		} else {
			sprite_mirror_x = true;
			if (sprite_mirror_y) {
				src += sprite_h * (pos_x + sprite_w - 1);
				pos_x = 0;
			} else {
				src += pos_x + sprite_w - 1;
				sprite_mirror_x = true;
				pos_x = 0;
			}
		}
	}
	if (sprite_clipped_w <= 0) {
		return;
	}

	int16_t sprite_clipped_h;
	if (pos_y >= 0) {
		if (pos_y < 224 - sprite_h) {
			sprite_clipped_h = sprite_h;
		} else {
			sprite_clipped_h = 224 - pos_y;
		}
	} else {
		sprite_clipped_h = sprite_h + pos_y;
		if (sprite_mirror_y) {
			src -= pos_y;
		} else {
			src -= sprite_w * pos_y;
		}
		pos_y = 0;
	}
	if (sprite_clipped_h <= 0) {
		return;
	}

	if (!sprite_mirror_x && (flags & 2)) {
		if (sprite_mirror_y) {
			src += sprite_h * (sprite_w - 1);
		} else {
			src += sprite_w - 1;
		}
	}

	const uint32_t dst_offset = 256 * pos_y + pos_x;
	const uint8_t sprite_col_mask = ((flags & 0x60) == 0x60) ? 0x50 : 0x40;

//	emu_printf("dst_offset=0x%X src_offset=0x%X\n", dst_offset, src - dataPtr);

	if (!(flags & 2)) {
		if (sprite_mirror_y) {
			_vid.drawSpriteSub5(src, _vid._frontLayer + dst_offset, sprite_h, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub3(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	} else {
		if (sprite_mirror_y) {
			_vid.drawSpriteSub6(src, _vid._frontLayer + dst_offset, sprite_h, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		} else {
			_vid.drawSpriteSub4(src, _vid._frontLayer + dst_offset, sprite_w, sprite_clipped_h, sprite_clipped_w, sprite_col_mask);
		}
	}
//	_vid.markBlockAsDirty(pos_x, pos_y, sprite_clipped_w, sprite_clipped_h, _vid._layerScale);
}
*/
int Game::loadMonsterSprites(LivePGE *pge) {
//	debug(DBG_GAME, "Game::loadMonsterSprites()");
	const InitPGE *init_pge = pge->init_PGE;
	if (init_pge->obj_node_number != 0x49 && init_pge->object_type != 10) {
		return 0xFFFF;
	}
	if (init_pge->obj_node_number == _curMonsterFrame) {
		return 0xFFFF;
	}
	if (pge->room_location != _currentRoom) {
		return 0;
	}

	const uint8_t *mList = _monsterListLevels[_currentLevel];
	while (*mList != init_pge->obj_node_number) {
		if (*mList == 0xFF) { // end of list
			return 0;
		}
		mList += 2;
	}
	_curMonsterFrame = mList[0];
	if (_curMonsterNum != mList[1]) {
		_curMonsterNum = mList[1];
/*		switch (_res._type) {
		case kResourceTypeDOS: {
				const char *name = _monsterNames[0][_curMonsterNum];
				_res.load(name, Resource::OT_SPRM);
//				_res.load_SPR_OFF(name, _res._sprm);
//				_vid.setPaletteSlotLE(5, _monsterPals[_curMonsterNum]);
			}
			break;

		case kResourceTypeMac: {
*/ 				Color palette[512];
//				Color *palette = (Color *)hwram_ptr;
//				_cut._stop=true; // vbt bidouille pour relancer la piste audio

// on l'appelle juste pour la palette				
				_res.MAC_loadMonsterData(_monsterNames[0][_curMonsterNum], palette);
				static const int kMonsterPalette = 5;
				for (int i = 0; i < 16; ++i) {
					const int color = 256 + kMonsterPalette * 16 + i;
					_stub->setPaletteEntry(color, &palette[color]);
				}
//			}
//			break;
//		}
	}
	return 0xFFFF;
}

bool Game::hasLevelRoom(int level, int room) const {
//	emu_printf("Game::hasLevelMap() level %d room%d\n", level, room);
//	if (_res._type == kResourceTypeMac) {
		return _res.MAC_hasLevelMap(level, room);
/*	}
	if (_res._map) {
		return READ_LE_UINT32(_res._map + room * 6) != 0;
	} else if (_res._lev) {
		return READ_BE_UINT32(_res._lev + room * 4) > 0x100;
	}
	return false;*/
}
/*
static bool isMetro(int level, int room) {
	return level == 1 && (room == 0 || room == 13 || room == 38 || room == 51);
}
*/


void Game::loadLevelRoom() {
//	emu_printf("Game::loadLevelMap() room=%d\n", _currentRoom);
	_vid.MAC_decodeMap(_currentLevel, _currentRoom);
}

void Game::loadLevelData() {
	_res.clearLevelRes();
	const Level *lvl = &_gameLevels[_currentLevel];
//	switch (_res._type) {
#if 0		
	case kResourceTypeDOS:
		_res.load(lvl->name, Resource::OT_MBK);
		_res.load(lvl->name, Resource::OT_CT);
		_res.load(lvl->name, Resource::OT_PAL);
		_res.load(lvl->name, Resource::OT_RP);
		/*if (_res._isDemo || g_options.use_tile_data || _res._aba) { // use .BNQ/.LEV/(.SGD) instead of .MAP (PC demo)
			if (_currentLevel == 0) {
				_res.load(lvl->name, Resource::OT_SGD);
			}
			_res.load(lvl->name, Resource::OT_LEV);
			_res.load(lvl->name, Resource::OT_BNQ);
		} else*/ {
			_res.load(lvl->name, Resource::OT_MAP);
		}
		_res.load(lvl->name2, Resource::OT_PGE);
		_res.load(lvl->name2, Resource::OT_OBJ);
		_res.load(lvl->name2, Resource::OT_ANI);
		_res.load(lvl->name2, Resource::OT_TBN);
		break;
#endif
//	case kResourceTypeMac:
		SAT_cleanRAM (HWRAM|LWRA1|VRAM1);
//heapWalk();		
		sat_free(_res._spc); // on ne vire pas
		sat_free(_res._ani);
//emu_printf("2chwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
		_res.MAC_unloadLevelData();
		sat_free(_res._cmd);
		sat_free(_res._pol);
		sat_free(_res._cine_off);

		_vid.setTextPalette();
//		_stub->copyRect(0, 0, _vid._w, 16, _vid._frontLayer, _vid._w);
		SAT_preloadCDfiles();
		slScrAutoDisp(NBG1ON);
//		_stub->copyRect(0, 0, _vid._w, 16, _vid._frontLayer, _vid._w);
		_res.MAC_loadLevelData(_currentLevel);
		SAT_preloadMonsters();
		SAT_preloadSpc();
		slScrAutoDisp(NBG0ON|NBG1ON|SPRON);
//		break;
//	}
	_cut._id = (int8_t)lvl->cutscene_id;

	_curMonsterNum = 0xFFFF;
	_curMonsterFrame = 0;
//	_printLevelCodeCounter = 150;

	_col_slots2Cur = _col_slots2;
	_col_slots2Next = 0;

	memset(_pge_liveTable2, 0, sizeof(_pge_liveTable2));
	memset(_pge_liveTable1, 0, sizeof(_pge_liveTable1));

	_currentRoom = _res._pgeInit[0].init_room;
	uint16_t n = _res._pgeNum;
 /// vbt : pour afficher les bgs	
 /*
slScrAutoDisp(NBG0ON|NBG1ON|SPRON);	
	for (int i = _currentRoom;i <  _currentRoom+n;i++)
	{
		_vid.MAC_decodeMap(_currentLevel, i);
		char str[60];
		sprintf(str,"room %02d",i);
		_vid.drawString(str, 4, 60, 0xE7);

		_vid.SAT_displayPalette();
	}
	slScrAutoDisp(NBG1ON|SPRON);
emu_printf("pge_loadForCurrentLevel %d\n",n);	
*/
	while (n--) {
		pge_loadForCurrentLevel(n);
	}
#ifdef DEMO
	if (_demoBin != -1) {
		_cut._id = 0xFFFF;
		_pgeLive[0].room_location = _demoInputsMac[_demoBin].room;
		_pgeLive[0].pos_x = _demoInputsMac[_demoBin].x;
		_pgeLive[0].pos_y = _demoInputsMac[_demoBin].y;
		_inp_demPos = 0;
//		_printLevelCodeCounter = 0;
	}
#endif
	for (uint16_t i = 0; i < _res._pgeNum; ++i) {
		if (_res._pgeInit[i].skill <= _skillLevel) {
			LivePGE *pge = &_pgeLive[i];
			pge->next_PGE_in_room = _pge_liveTable1[pge->room_location];
			_pge_liveTable1[pge->room_location] = pge;
		}
	}
	pge_resetMessages();
	_validSaveState = false;
	memset4_fast(&_vid._frontLayer[0],0x00,_vid._w* 100);
	_stub->copyRect(0, 0, _vid._w, 100, _vid._frontLayer, _vid._w);
//emu_printf("2xhwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
}

void Game::drawIcon(uint8_t iconNum, int16_t x, int16_t y, uint8_t colMask) {
//	uint8_t buf[16 * 16];
/*	switch (_res._type) {
	case kResourceTypeDOS:
//		_vid.PC_decodeIcn(_res._icn, iconNum, buf);  // vbt ࠲emettre
		break;
	case kResourceTypeMac:
		switch (iconNum) {
		case 76: // cursor
			iconNum = 32;
			break;
		case 77: // up
			iconNum = 33;
			break;
		case 78: // down
			iconNum = 34;
			break;
		}*/
		_vid.MAC_drawSprite(x, y, _res._icn, iconNum, 0, false);
//		return;
//	}
//	_vid.drawSpriteSub1(buf, _vid._frontLayer + x + y * _vid._w, 16, 16, 16, colMask << 4);
//	_vid.markBlockAsDirty(x, y, 16, 16, _vid._layerScale);
}

int channel_len[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void Game::playSound(uint8_t num, uint8_t softVol) {
	if (num < NUM_SFXS) {
		SoundFx *sfx = &_res._sfxList[num];
		if (sfx->data) {
//		emu_printf("play sound %02d volume : softVol %d calc : %d\n",num,softVol, (Mixer::MAX_VOLUME >> (2 * softVol))-1);
			const int volume = (Mixer::MAX_VOLUME >> (2 * softVol))-1;

			int i=0;
// vbtvbt
// 16 & 17 pour cdda
			for(i=0;i<16;i++)
			{
//				if(i==16 || i==17)
//					continue;
				
//				pcm_sample_stop(i);
				if(channel_len[i]==0)
				{
					if(i<15)
					pcm_sample_stop(i+1);
					else
					pcm_sample_stop(0);
						
					break;
				}
			}

			if(i>15)
			{
				/*volatile scsp_slot_regs_t *slot1 = (scsp_slot_regs_t *)get_scsp_slot(i-1);
				slot1->kyonb = 0;
				slot1->kyonex = 0;
				asm("nop");	*/			
				i=0;
				memset(channel_len,0,16*sizeof(int));
//				memset(&channel_len[17],0,14*sizeof(int));
			}

//			emu_printf("play sound %02d/%d on channel %02d len %d\n",num,_res._numSfx,i,sfx->len);
			channel_len[i] = sfx->len;

			uint32_t address = (uint32_t)sfx->data;
			pcm_sample_t pcm = {.addr = address, .vol = volume, .bit = pcm_sample_8bit};
//		pcm_sample_stop(&pcm);
			pcm_prepare_sample(&pcm, i, sfx->len);
//			pcm_sample_set_samplerate(&pcm, sfx->freq);
			pcm_sample_set_samplerate(i, 6000);
			pcm_sample_set_loop(i, pcm_sample_loop_no_loop);
			pcm_sample_start(i);
		}
	} else if (num == 66) {
		// open/close inventory (DOS)
	} else if (num >= 68 && num <= 75) {
//		emu_printf("play sfx %d\n",num);
		
		// in-game music
//		_sfxPly.play(num);
// 		_mix.playMusic(num); // vbt ࠶oir entre les 2
	} else if (num == 76) {
		// metro
	} else if (num == 77) {
		// triggered when Conrad draw his gun
	} else {
//		warning("Unknown sound num %d", num);
	}
}

uint16_t Game::getRandomNumber() {
/*	uint32_t n = _randSeed * 2;
	if (((int32_t)_randSeed) >= 0) {
		n ^= 0x1D872B41;
	}
	_randSeed = n;
	return n & 0xFFFF;
*/
/*	
    _randSeed = (_randSeed * 16807) % 2147483647;
    return (_randSeed >> 16) & 0xFFFF;
*/
// Compute next seed: X(n+1) = (a * X(n) + c) mod m
#define MULTIPLIER 69069 // Common multiplier for good period
#define INCREMENT 1      // Odd increment for LCG
#define MODULUS 65536    // 2^16 for 16-bit modulus

    _randSeed = (MULTIPLIER * _randSeed + INCREMENT) % MODULUS;
    
    // Convert to signed 16-bit range (-32768 to 32767)
    return (int16_t)(_randSeed & 0xFFFF) - 32768;
}

void Game::changeLevel() {
//	_vid.fadeOut();  // create garbage at the end of level 4
//		slTVOn();
//	clearStateRewind();
	loadLevelData();
//emu_printf("3hwram free %08d lwram used %08d lwram2 %08d\n",end1-(int)hwram_ptr,(int)current_lwram-0x200000,_vid._layerSize);
	loadLevelRoom();
	_vid.setPalette0xF();
	_vid.setTextPalette();
//	_vid.fullRefresh();
}

void Game::handleInventory() {
	LivePGE *selected_pge = 0;
	LivePGE *pge = &_pgeLive[0];
	if (pge->life > 0 && pge->current_inventory_PGE != 0xFF) {
		playSound(66, 0);
		InventoryItem items[24];
		int num_items = 0;
		uint8_t inv_pge = pge->current_inventory_PGE;
		while (inv_pge != 0xFF) {
			items[num_items].icon_num = _res._pgeInit[inv_pge].icon_num;
			items[num_items].init_pge = &_res._pgeInit[inv_pge];
			items[num_items].live_pge = &_pgeLive[inv_pge];
			inv_pge = _pgeLive[inv_pge].next_inventory_PGE;
			++num_items;
		}
		items[num_items].icon_num = 0xFF;
		int current_item = 0;
		int num_lines = (num_items - 1) / 4 + 1;
		int current_line = 0;
		bool display_score = false;

		while (!_stub->_pi.backspace && !_stub->_pi.quit & !_cut._interrupted) {
			static const int icon_spr_w = 16;
			static const int icon_spr_h = 16;
/*			switch (_res._type) {
			case kResourceTypeDOS: {
					// draw inventory background
					int icon_num = 31;
					for (int y = 140; y < 140 + 5 * icon_spr_h; y += icon_spr_h) {
						for (int x = 56; x < 56 + 9 * icon_spr_w; x += icon_spr_w) {
							drawIcon(icon_num, x, y, 0xF);
							++icon_num;
						}
					}
				}
				break;
			case kResourceTypeMac:*/
//				drawIcon(31, 56, 140, 0xF);
				_vid.MAC_drawFG(56,140,_res._icn, 31);
/*				break;
			}*/
			if (!display_score) {
				int icon_x_pos = 72;
				for (int i = 0; i < 4; ++i) {
					int item_it = current_line * 4 + i;
					if (items[item_it].icon_num == 0xFF) {
						break;
					}
//					drawIcon(items[item_it].icon_num, icon_x_pos, 157, 0xA);
					_vid.MAC_drawFG(icon_x_pos,157,_res._icn, items[item_it].icon_num);
					if (current_item == item_it) {
//						drawIcon(76, icon_x_pos, 157, 0xA);
						_vid.MAC_drawFG(icon_x_pos,157,_res._icn, 32); //76
						selected_pge = items[item_it].live_pge;
						uint8_t txt_num = items[item_it].init_pge->text_num;
						const uint8_t *str = _res.getTextString(_currentLevel, txt_num);
						drawString(str, Video::GAMESCREEN_W, 189, 0xED, true);
						if (items[item_it].init_pge->init_flags & 4) {
							char buf[10];
							snprintf(buf, sizeof(buf), "%d", selected_pge->life);
							_vid.drawString(buf, (Video::GAMESCREEN_W - strlen(buf) * Video::CHAR_W) / 2, 197, 0xED);
						}
					}
					icon_x_pos += 32;
				}
				if (current_line != 0) {
//					drawIcon(77, 120, 143, 0xA); // up arrow
					_vid.MAC_drawFG(120,143,_res._icn, 33);//77
				}
				if (current_line != num_lines - 1) {
//					drawIcon(78, 120, 176, 0xA); // down arrow
					_vid.MAC_drawFG(120,176,_res._icn, 34);//78
				}
			} else {
				char buf[50];
				snprintf(buf, sizeof(buf), "SCORE %08u", _score);
				_vid.drawString(buf, (114 - strlen(buf) * Video::CHAR_W) / 2 + 72, 158, 0xE5);
				snprintf(buf, sizeof(buf), "%s:%s", _res.getMenuString(LocaleData::LI_06_LEVEL), _res.getMenuString(LocaleData::LI_13_EASY + _skillLevel));
				_vid.drawString(buf, (114 - strlen(buf) * Video::CHAR_W) / 2 + 72, 166, 0xE5);
				/*if (0) { // if the protection screen code was not properly cracked...
					static const uint8_t kCrackerText[17] = {
						0x19, 0x08, 0x1B, 0x19, 0x11, 0x1F, 0x08, 0x67, 0x18,
						0x16, 0x1B, 0x13, 0x08, 0x1F, 0x1B, 0x0F, 0x5A
					};
					for (int i = 0; i < 17; ++i) {
						buf[i] = kCrackerText[i] ^ 0x5A;
					}
					_vid.drawString(buf, 65, 193, 0xE4);
				}*/
			}
			_stub->copyRect(112, 280, 288, 144, _vid._frontLayer, _vid._w);
			_stub->updateScreen(0);
			_stub->sleep(80);
#ifdef DEMO
			inp_update();
#endif
			if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				if (current_line < num_lines - 1) {
					++current_line;
					current_item = current_line * 4;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
				if (current_line > 0) {
					--current_line;
					current_item = current_line * 4;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				if (current_item > 0) {
					int item_num = current_item % 4;
					if (item_num > 0) {
						--current_item;
					}
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				if (current_item < num_items - 1) {
					int item_num = current_item % 4;
					if (item_num < 3) {
						++current_item;
					}
				}
			}
			if (_stub->_pi.enter) {
				_stub->_pi.enter = false;
				display_score = !display_score;
			}
			if (_stub->_pi.escape) {   // vbt désactive le menu de load/save si dans menu ingame
				_stub->_pi.escape = false;
			}
//			slSynch();
		}
		// vbt : n'efface que le menu
		memset4_fast(&_vid._frontLayer[280*512],0x00, _vid._w*144);
		_stub->copyRect(112, 280, 288, 144, _vid._frontLayer, _vid._w);
		_stub->updateScreen(0);
		_stub->_pi.backspace = false;
		if (selected_pge) {
			pge_setCurrentInventoryObject(selected_pge);
		}
		playSound(66, 0);

		frame_y = frame_x = 0;
		frame_z = 30;		
	}
}
#ifdef DEMO
void Game::inp_update() {
//	_stub->processEvents();
	if (_demoBin != -1 && _inp_demPos < _res._demLen) {
		const int keymask = _res._dem[_inp_demPos++];
		
		_stub->_pi.dirMask = keymask & 0xF;
		_stub->_pi.enter = (keymask & 0x10) != 0; //PAD_PUSH_C // use item
		_stub->_pi.space = (keymask & 0x20) != 0; //PAD_PUSH_A // draw gun
		_stub->_pi.shift = (keymask & 0x40) != 0; //PAD_PULL_B // action
		_stub->_pi.backspace = (keymask & 0x80) != 0; //PAD_PUSH_START // inventory
		
//emu_printf("Item %d gun %d action/run %d invent %d dir %x\n", _stub->_pi.enter, _stub->_pi.space, _stub->_pi.shift,_stub->_pi.backspace, _stub->_pi.dirMask);

/*
		DIR_UP    = 1 << 0,
		DIR_DOWN  = 1 << 1,
		DIR_LEFT  = 1 << 2,
		DIR_RIGHT = 1 << 3
*/
		
	}
}
#endif
/*
void Game::makeGameDemoName(char *buf) {
	sprintf(buf, "rs-level%d.demo", _currentLevel + 1);
}
*/
void Game::makeGameStateName(uint8 slot, char *buf) {
	sprintf(buf, "rs%d-%02d", _currentLevel + 1, slot);
}

bool Game::saveGameState(uint8 slot) {
	bool success = false;
	char stateFile[8];
	char hdrdesc[10];
	makeGameStateName(slot, stateFile);
	sprintf(hdrdesc, "RS:%d-%d", _currentLevel + 1, _currentRoom);

	// Needed structs
	BupConfig conf[3];
	BupStat sttb;
	BupDir writetb;
	BupDate datetb;
	Uint8 *time;
//	Uint32 libBakBuf[4096] ;
//	Uint32 BackUpRamWork[2048];
	memset(&sbuf, 0, sizeof(SAVE_BUFFER));

	Uint8  *rle_buf		 = (Uint8  *)SCRATCH; //_res._scratchBuffer;
	Uint32 *libBakBuf    = (Uint32 *)(SCRATCH+10000);
	Uint32 *BackUpRamWork= (Uint32 *)(SCRATCH+10000+0x4000);
	sbuf.buffer	 		 = (Uint8  *)(SCRATCH+10000+0x6000);

	// SAVE INSTR. HERE!
	saveState(&sbuf);
	int cmprSize = LZ_Compress(sbuf.buffer, rle_buf, sbuf.idx);

	PER_SMPC_RES_DIS(); // Disable reset
		BUP_Init(libBakBuf, BackUpRamWork, conf);
		if( BUP_Stat(0, 0, &sttb) == BUP_UNFORMAT) 
		{	
			BUP_Format(0);
		}
	PER_SMPC_RES_ENA(); // Enable reset

	if (sttb.freeblock > 0) { // Not sure of the size of a block
		memset(writetb.filename, 0, 9);
		memset(writetb.comment, 0, 9);

		strncpy((char*) writetb.filename, stateFile, 11);
		strncpy((char*) writetb.comment, hdrdesc, 10);

		writetb.language = BUP_ENGLISH;

		time = PER_GET_TIM();

		datetb.year = (Uint8) (
					  (Uint16)(time[6] >> 4)   * 1000
					 +(Uint16)(time[5] >> 4)   * 10
					 +(Uint16)(time[5] & 0x0F) - 1980);
		datetb.month = time[4] & 0x0F;
		datetb.day = ((time[3] >> 4) * 10) + (time[3] & 0x0F);
		datetb.time = ((time[2] >> 4) * 10) + (time[2] & 0x0F);
		datetb.min = ((time[1] >> 4) * 10) + (time[1] & 0x0F);

		writetb.date = BUP_SetDate(&datetb);
		writetb.datasize = (cmprSize) + 1;

		Sint32 verify;
		PER_SMPC_RES_DIS(); // Disable reset
			BUP_Write(0, &writetb, rle_buf, OFF);
			verify = BUP_Verify(0, writetb.filename, rle_buf);
		PER_SMPC_RES_ENA(); // Enable reset

		if (verify == 0)
			success = true;
	}
	memset4_fast(&_vid._frontLayer[0], 0x00, _vid._layerSize); // utilisé pour la compression
	return success;
}

bool Game::loadGameState(uint8 slot) {
//	bool success = false;
	char stateFile[8];
	makeGameStateName(slot, stateFile);

	BupConfig conf[3];
	BupDir	dir[1];
//	Uint32 libBakBuf[4096];
//	Uint32 BackUpRamWork[2048];
	
	memset(&sbuf, 0, sizeof(SAVE_BUFFER));

	Uint8  *rle_buf		 = (Uint8  *)SCRATCH; //_res._scratchBuffer;
	Uint32 *libBakBuf    = (Uint32 *)(SCRATCH+10000);
	Uint32 *BackUpRamWork= (Uint32 *)(SCRATCH+10000+0x4000);
	sbuf.buffer	 		 = (Uint8  *)(SCRATCH+10000+0x6000);
	memset(rle_buf, 0, 40000);

	Uint32 i;
	int32 status;

	// Load save from saturn backup memory
	PER_SMPC_RES_DIS(); // Disable reset
		BUP_Init(libBakBuf, BackUpRamWork, conf);
		status = BUP_Read(0, (Uint8*)stateFile, rle_buf);
	PER_SMPC_RES_ENA(); // Enable reset
	if (status != 0)
		return false;

	BUP_Dir(0, (Uint8*)stateFile, 1, dir);

	int cmprSize = dir[0].datasize;
//emu_printf("cmprSize %d\n",cmprSize);
	LZ_Uncompress(rle_buf, sbuf.buffer, cmprSize);
	loadState(&sbuf);
	return true;
}

void Game::saveState(SAVE_BUFFER *sbuf) {
	sbuf->buffer[sbuf->idx] = _skillLevel; sbuf->idx++;
	WRITE_UINT32((sbuf->buffer + sbuf->idx), _score); sbuf->idx += 4;

	if (_col_slots2Cur == 0) {
		WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
	} else {
		WRITE_UINT32((sbuf->buffer + sbuf->idx), (_col_slots2Cur - &_col_slots2[0])); sbuf->idx += 4;
	}
	if (_col_slots2Next == 0) {
		WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
	} else {
		WRITE_UINT32((sbuf->buffer + sbuf->idx), (_col_slots2Next - &_col_slots2[0])); sbuf->idx += 4;
	}
	for (int i = 0; i < _res._pgeNum; ++i) {
		LivePGE *pge = &_pgeLive[i];
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->obj_type); sbuf->idx += 2;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->pos_x); sbuf->idx += 2;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->pos_y); sbuf->idx += 2;
		sbuf->buffer[sbuf->idx] = pge->anim_seq; sbuf->idx++;
		sbuf->buffer[sbuf->idx] = pge->room_location; sbuf->idx++;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->life); sbuf->idx += 2;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->counter_value); sbuf->idx += 2;
		sbuf->buffer[sbuf->idx] = pge->collision_slot; sbuf->idx++;
		sbuf->buffer[sbuf->idx] = pge->next_inventory_PGE; sbuf->idx++;
		sbuf->buffer[sbuf->idx] = pge->current_inventory_PGE; sbuf->idx++;
		sbuf->buffer[sbuf->idx] = pge->ref_inventory_PGE; sbuf->idx++;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->anim_number); sbuf->idx += 2;
		sbuf->buffer[sbuf->idx] = pge->flags; sbuf->idx++;
		sbuf->buffer[sbuf->idx] = pge->index; sbuf->idx++;
		WRITE_UINT16((sbuf->buffer + sbuf->idx), pge->first_obj_number); sbuf->idx += 2;
		if (pge->next_PGE_in_room == 0) {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
		} else {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), (pge->next_PGE_in_room - &_pgeLive[0])); sbuf->idx += 4;
		}
		if (pge->init_PGE == 0) {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
		} else {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), (pge->init_PGE - &_res._pgeInit[0])); sbuf->idx += 4;
		}
	}
	
	Uint32 idx = 0;
	for(idx = 0; idx < 0x1C00; idx++) {
		sbuf->buffer[sbuf->idx] = _res._ctData[0x100 + idx]; sbuf->idx++;
	}
	//f->write(&_res._ctData[0x100], 0x1C00);
	for (CollisionSlot2 *cs2 = &_col_slots2[0]; cs2 < _col_slots2Cur; ++cs2) {
		if (cs2->next_slot == 0) {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
		} else {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), (cs2->next_slot - &_col_slots2[0])); sbuf->idx += 4;
		}
		if (cs2->unk2 == 0) {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), 0xFFFFFFFF); sbuf->idx += 4;
		} else {
			WRITE_UINT32((sbuf->buffer + sbuf->idx), (cs2->unk2 - &_res._ctData[0x100])); sbuf->idx += 4;
		}
		sbuf->buffer[sbuf->idx] = cs2->data_size; sbuf->idx++;
		//f->write(cs2->data_buf, 0x10);
		for(idx = 0; idx < 0x10; idx++) {
			sbuf->buffer[sbuf->idx] = (cs2->data_buf[idx]); sbuf->idx++;
		}
	}
	WRITE_UINT16((sbuf->buffer + sbuf->idx), _pge_opGunVar);
}

void Game::loadState(SAVE_BUFFER *sbuf) {
	uint16 i;
	uint32 off;
	_skillLevel = sbuf->buffer[sbuf->idx]; sbuf->idx++;
	_score = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
	memset(_pge_liveTable2, 0, sizeof(_pge_liveTable2));
	memset(_pge_liveTable1, 0, sizeof(_pge_liveTable1));
	off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
	if (off == 0xFFFFFFFF) {
		_col_slots2Cur = 0;
	} else {
		_col_slots2Cur = &_col_slots2[0] + off;
	}
	off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
	if (off == 0xFFFFFFFF) {
		_col_slots2Next = 0;
	} else {
		_col_slots2Next = &_col_slots2[0] + off;
	}
	for (i = 0; i < _res._pgeNum; ++i) {
		LivePGE *pge = &_pgeLive[i];
		pge->obj_type = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->pos_x = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->pos_y = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->anim_seq = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->room_location = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->life = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->counter_value = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->collision_slot = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->next_inventory_PGE = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->current_inventory_PGE = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->ref_inventory_PGE = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->anim_number = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		pge->flags = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->index = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		pge->first_obj_number = READ_LE_UINT16(sbuf->buffer + sbuf->idx); sbuf->idx += 2;
		off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
		if (off == 0xFFFFFFFF) {
			pge->next_PGE_in_room = 0;
		} else {
			pge->next_PGE_in_room = &_pgeLive[0] + off;
		}
		off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
		if (off == 0xFFFFFFFF) {
			pge->init_PGE = 0;
		} else {
			pge->init_PGE = &_res._pgeInit[0] + off;
		}
	}
	
	Uint32 idx;
	for(idx = 0; idx < 0x1C00; idx++) {
		_res._ctData[0x100 + idx] = sbuf->buffer[sbuf->idx]; sbuf->idx++;
	}
	for (CollisionSlot2 *cs2 = &_col_slots2[0]; cs2 < _col_slots2Cur; ++cs2) {
		off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
		if (off == 0xFFFFFFFF) {
			cs2->next_slot = 0;
		} else {
			cs2->next_slot = &_col_slots2[0] + off;
		}
		off = READ_LE_UINT32(sbuf->buffer + sbuf->idx); sbuf->idx += 4;
		if (off == 0xFFFFFFFF) {
			cs2->unk2 = 0;
		} else {
			cs2->unk2 = &_res._ctData[0x100] + off;
		}
		cs2->data_size = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		for(idx = 0; idx < 0x10; idx++) {
			cs2->data_buf[idx] = sbuf->buffer[sbuf->idx]; sbuf->idx++;
		}
	}
	for (i = 0; i < _res._pgeNum; ++i) {
		if (_res._pgeInit[i].skill <= _skillLevel) {
			LivePGE *pge = &_pgeLive[i];
			if (pge->flags & 4) {
				_pge_liveTable2[pge->index] = pge;
			}
			pge->next_PGE_in_room = _pge_liveTable1[pge->room_location];
			_pge_liveTable1[pge->room_location] = pge;
		}
	}
	resetGameState();
}

void Game::clearSaveSlots(uint8 level) {
	BupConfig conf[3];
	BupStat sttb;

	Uint32 *libBakBuf    = (Uint32 *)(hwram_screen+SAV_BUFSIZE);
	Uint32 *BackUpRamWork= (Uint32 *)(hwram_screen+SAV_BUFSIZE+0x4000);

	PER_SMPC_RES_DIS(); // Disable reset
		BUP_Init(libBakBuf, BackUpRamWork, conf);
		if( BUP_Stat(0, 0, &sttb) == BUP_UNFORMAT) BUP_Format(0);
	PER_SMPC_RES_ENA(); // Enable reset
	
	char filename[20];

	for(uint8 idx_lev = 0; idx_lev < 6; idx_lev++) {
		if(idx_lev == level) continue;
	
		for(uint8 idx_slot = 0; idx_slot < 4; idx_slot++) {
			sprintf(filename, "rs%d-%02d", idx_lev + 1, idx_slot);
			BUP_Delete(0, (Uint8*)filename);
		}
	}
}

void AnimBuffers::addState(uint8_t stateNum, int16_t x, int16_t y, const uint8_t *dataPtr, LivePGE *pge, uint8_t w, uint8_t h) {
//	emu_printf("AnimBuffers::addState() stateNum=%d x=%d y=%d dataPtr=%p pge=%p\n", stateNum, x, y, dataPtr, pge);
//	assert(stateNum < 4);
	if(stateNum >= 4)
		return;
	
	AnimBufferState *state = _states[stateNum];
	state->x = x;
	state->y = y;
	state->w = w;
	state->h = h;
	state->dataPtr = dataPtr;
	state->pge = pge;
	++_curPos[stateNum];
	++_states[stateNum];
}
#ifdef DEBUG2
void debugSpriteDisplay(SAT_sprite* sprData, DecodeBuffer& buf, int j, int count) {
    buf.x = 200 - sprData->x;
    buf.y = 240 - sprData->y;
    char debug_info[60];
    sprintf(debug_info, "%03d/%03d 0x%08x %d %d ", j, count - 1, sprData->cgaddr, buf.dst_w, buf.dst_h);
    _vid.drawString(debug_info, 4, 60, 0xE7);
    _stub->copyRect(0, 20, _vid._w, 16, _vid._frontLayer, _vid._w);
    memset4_fast(&_vid._frontLayer[40 * _vid._w], 0x00, _vid._layerSize);
    _vid.SAT_displaySprite(*sprData, buf);
    slSynch();
}
#endif

void Game::SAT_loadSpriteData(const uint8_t* spriteData, int baseIndex, uint8_t* destPtr) 
{
    const int count = READ_BE_UINT16(spriteData + 2);
    DecodeBuffer buf{};
//    buf.setPixel = setPixelFunc;
//    buf.type = (setPixelFunc == _vid.MAC_setPixel4Bpp) ? 1 : 0;

    const bool isSpc = (spriteData == _res._spc);
    const bool isMonster = (spriteData == _res._monster);

    buf.type = (isMonster) ? 1 : 0;

    for (unsigned int j = 0; j < count; j++) {
        const uint8_t* dataPtr = _res.MAC_getImageData(spriteData, j);
        if (!dataPtr) continue;

        uint16_t width = READ_BE_UINT16(dataPtr + 2) & 0xff;
        uint16_t height_raw = READ_BE_UINT16(dataPtr);
        uint16_t height = (height_raw + 7) & ~7;
        uint16_t xPos = READ_BE_UINT16(dataPtr + 4);
        uint16_t yPos = READ_BE_UINT16(dataPtr + 6);

        buf.dst_w = width;
        buf.dst_h = height;
        buf.ptr = destPtr;

        if (isSpc && j != SPR_METRO && j != SPR_ELEVATOR) continue;

        memset(buf.ptr, 0, width * height); // Optimize if possible
        _res.MAC_decodeImageData(spriteData, j, &buf, 0xff);

        SAT_sprite* sprData = &_res._sprData[baseIndex + j];
        sprData->color = isMonster ? 5 : -1;

		if(j == SPR_METRO)
		{
/*
palette utilisée
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x09,
0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x20
0x20=> couleur 245 (couleur 6)
*/
			for ( uint16_t i = 0; i < width * height;i++)
			{
				if (destPtr[i] != 0)
				{
					if(destPtr[i]<0x20)
						destPtr[i]+=239;
					else
						destPtr[i]=245;
					if(destPtr[i]==254)
						destPtr[i]=246;
				}
//				else
				if (destPtr[i] == 240)
					destPtr[i] = 255; // 4bpp only
			}
			_vid.convert_8bpp_to_4bpp_inplace(destPtr, width * height);
			sprData->color = 15;
//			buf.type = 1;
		}

        sprData->size = (height / 8) << 8 | width;
        sprData->x_flip = (uint8_t)-(xPos - height_raw - 1 - (height - height_raw));
        sprData->x = xPos;
        sprData->y = yPos;

        size_t dataSize = SAT_ALIGN((width * height) / (buf.type == 1 ? 2 : 1));

#ifdef REDUCE_4BPP
        if (isSpc) {
            int min_val = 256, max_val = 0;
            size_t pixel_count = width * height;
            for (int i = 0; i < pixel_count; i++) {
                uint8_t val = buf.ptr[i];
                if (val != 0 && val < min_val) min_val = val;
                if (val != 255 && val > max_val) max_val = val;
            }
            if ((max_val - (min_val >> 4) * 16) < 16) {
                for (int k = 0; k < pixel_count; k += 2) {
                    uint8_t value1 = buf.ptr[k + 1];
                    uint8_t value2 = buf.ptr[k];
                    buf.ptr[k / 2] = (value1 & 0x0f) | ((value2 & 0x0f) << 4);
                }
                dataSize = SAT_ALIGN(pixel_count / 2);
                sprData->color = (min_val >> 4);
            }
        }
#endif

        void* target;
        int cgaddr;
        if ((position_vram + dataSize) <= VRAM_MAX || j == SPR_METRO || j == SPR_ELEVATOR) {
            TEXTURE tx = TEXDEF(width, height, position_vram);
            target = (void*)(SpriteVRAM + (tx.CGadr << 3));
            cgaddr = (int)tx.CGadr;
            position_vram += dataSize;
            position_vram_aft_monster = position_vram;
        } else {
            target = current_lwram;
            cgaddr = (int)target;
            current_lwram += SAT_ALIGN(dataSize);
        }
        DMA_ScuMemCopy(target, buf.ptr, dataSize);
        sprData->cgaddr = cgaddr;

#ifdef DEBUG2
        debugSpriteDisplay(sprData, buf, j, count);
#endif
    }
}

/*
void Game::SAT_preloadIcon()
{
		Color clut[512];
		_res.MAC_setupRoomClut(_currentLevel, _currentRoom, clut);		

		const int baseColor = 256;
		for (int i = 0; i < 256; ++i) {
			int color = baseColor + i;
			_stub->setPaletteEntry(color, &clut[color]);
		}
					
	for (int i = 0; i <= 34;i++)
	{
//		int frame = _res.MAC_getPersoFrame(i);
		
//		if(frame>=0)
			_vid.MAC_drawSprite(60, 60, _res._icn, i, 0, 0);
		slSynch();
	}
}

void Game::SAT_preloadPerso()
{
		Color clut[512];
		_res.MAC_setupRoomClut(_currentLevel, _currentRoom, clut);		

		const int baseColor = 256;
		for (int i = 0; i < 256; ++i) {
			int color = baseColor + i;
			_stub->setPaletteEntry(color, &clut[color]);
		}
					
	for (int i = 0; i <= 0x506;i++)
	{
		int frame = _res.MAC_getPersoFrame(i);
		
		if(frame>=0)
			_vid.MAC_drawSprite(60, 60, _res._perso, frame, 0, 0);
		slSynch();
	}
//	while(1);
}
*/
void Game::SAT_preloadMonsters() {
	_curMonsterNum = 0xFFFF;

	const uint8_t* mList = _monsterListLevels[_currentLevel];
#ifdef DEBUG
	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();
#endif
	while (*mList != 0xFF) {
		_curMonsterFrame = mList[0];

		if (_curMonsterNum != mList[1]) {
			_curMonsterNum = mList[1];

			static const struct {
				const char* id;
				const char* name;
				int index;
			} data[] = {
				{ "junky", "Junky", 0x22F },
				{ "mercenai", "Mercenary", 0x2EA },
				{ "replican", "Replicant", 0x387 },
				{ "glue", "Alien", 0x430 },
				{ 0, 0, 0 }
			};

//			sat_free(_res._monster);
//			_res._monster = 0;

#ifdef DEBUG2					
			Color palette[512];
			// on l'appelle juste pour la palette				
			_res.MAC_loadMonsterData(_monsterNames[0][_curMonsterNum], palette);
			static const int kMonsterPalette = 5;
			for (int j = 0; j < 16; ++j) {
				const int color = 256 + kMonsterPalette * 16 + j;
				_stub->setPaletteEntry(color, &palette[color]);
			}
#endif
			for (int i = 0; data[i].id; ++i) {
				if (strcmp(data[i].id, _monsterNames[0][_curMonsterNum]) == 0) {
#ifdef DEBUG
	unsigned int st = _stub->getTimeStamp();
#endif
					_res._monster = _res.decodeResourceMacData(data[i].name, true);
#ifdef DEBUG
	unsigned int se = _stub->getTimeStamp();
	emu_printf("--lzss %d enn : %d\n",i,se-st);	
#endif	
					SAT_loadSpriteData(_res._monster, data[i].index, hwram_screen);

					break; // Break out of the loop once the monster is found and processed.
				}
			}
		}
		mList += 2;
	}
#ifdef DEBUG
	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration enn : %d\n",e-s);
#endif
}

void Game::SAT_preloadSpc() {
#ifdef DEBUG2
		Color clut[512];
		_res.MAC_setupRoomClut(_currentLevel, _currentRoom, clut);		

		const int baseColor = 256;
		for (int i = 0; i < 256; ++i) {
			int color = baseColor + i;
			_stub->setPaletteEntry(color, &clut[color]);
		}
#endif
#ifdef DEBUG
	_stub->initTimeStamp();
	unsigned int s = _stub->getTimeStamp();
#endif
	SAT_loadSpriteData(_res._spc, _res.NUM_SPRITES, hwram_screen);
#ifdef DEBUG
	unsigned int e = _stub->getTimeStamp();
	emu_printf("--duration spc : %d\n",e-s);
#endif
}

#ifdef USE_SLAVE
volatile bool slave_done_flag = false;

typedef struct {
    Cutscene* cut; // Pointer to Resource instance
//    const char* current_lwram; // address in ram
} SlaveData;

void wait_for_slave()
{
    while (!*((volatile bool *)(&slave_done_flag + 0x20000000)));
    // Resetting the slave done flag to false for the next operation
    slave_done_flag = false;
}

static void process_commands(void* arg) {
    SlaveData* data = static_cast<SlaveData*>(arg);
    Cutscene* cut = data->cut;

    emu_printf("slSlaveFunc process_commands %p\n", 0x200000);
	cut->playSet((const uint8_t *)0x200000, 0x2B14);
    emu_printf("slSlaveFunc process_commands end\n");
    slave_done_flag = true;
}
#endif

void Game::SAT_preloadCDfiles() {
	_vid.drawString("Loading Please wait", 20, 40, 0xE5);
	_stub->copyRect(40, 80, _vid._w, 16, _vid._frontLayer, _vid._w);
#ifdef USE_SLAVE	
	_res.MAC_closeMainFile();
	GFS_Load(GFS_NameToId((int8_t *)"CDFILES.CMP"),0,(void *)current_lwram,21623);
	_res.MAC_reopenMainFile();
//	_cut.playSet(current_lwram, 0x2B14);
    SlaveData slaveData = { &_cut };
current_lwram+=SAT_ALIGN(21623);
    emu_printf("slSlaveFunc %p\n", current_lwram);
    // Pass process_commands and slaveData to slSlaveFunc
    slSlaveFunc(reinterpret_cast<void (*)()>(process_commands), &slaveData);
    emu_printf("wait_for_slave\n");
 //   wait_for_slave(); // Wait for slave to complete
    emu_printf("wait_for_slave end\n");
#endif
}

void Game::SAT_cleanRAM(unsigned char all) {
//emu_printf("ram clean %d\n",all);
		if (all & 1)
			hwram_ptr = hwram+HWRAM_SCREEN_SIZE;
		if (all & 8)
			position_vram = 0x1000;
		if (all & 4)
		{
//			emu_printf("current_lwram1 %x\n",VBT_L_START);
			current_lwram = (uint8_t *)VBT_L_START;
		}
		if (all & 2)
		{
//			emu_printf("current_lwram2 %p\n",save_current_lwram);
			current_lwram = (uint8_t *)save_current_lwram;
		}
}
