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

#ifndef __GAME_H__
#define __GAME_H__

#include "intern.h"
#include "cutscene.h"
#include "menu.h"
#include "mixer.h"
#include "mod_player.h"
#include "resource.h"
#include "sfx_player.h"
#include "video.h"

#define SAV_BUFSIZE (1024 * 30)

typedef struct {
	Uint8 buffer[SAV_BUFSIZE]; // 10 Kilobytes of save buffer
	Uint32 idx; // Index of current position inside the buffer
} SAVE_BUFFER;

struct File;
struct SystemStub;

enum {
	kCheatOneHitKill = 1 << 0,
	kCheatNoHit = 1 << 1,
	kCheatLifeCounter = 1 << 2
};

struct Game {
	typedef int (Game::*pge_OpcodeProc)(ObjectOpcodeArgs *args);
	typedef int (Game::*pge_ZOrderCallback)(LivePGE *, LivePGE *, uint8, uint8);
	typedef int (Game::*col_Callback1)(LivePGE *, LivePGE *, int16, int16);
	typedef int (Game::*col_Callback2)(LivePGE *, int16, int16, int16);

	enum {
		CT_UP_ROOM    = 0x00,
		CT_DOWN_ROOM  = 0x40,
		CT_RIGHT_ROOM = 0x80,
		CT_LEFT_ROOM  = 0xC0
	};

	static const Demo _demoInputs[3];
	static const Level _gameLevels[];
	static const uint16_t _scoreTable[];
	static const uint8_t _monsterListLevel1[];
	static const uint8_t _monsterListLevel2[];
	static const uint8_t _monsterListLevel3[];
	static const uint8_t _monsterListLevel4_1[];
	static const uint8_t _monsterListLevel4_2[];
	static const uint8_t _monsterListLevel5_1[];
	static const uint8_t _monsterListLevel5_2[];
	static const uint8_t *_monsterListLevels[];
	static const uint8_t _monsterPals[4][32];
	static const char *_monsterNames[2][4];
	static const pge_OpcodeProc _pge_opcodeTable[];
	static const uint8 _pge_modKeysTable[];
	static const uint8 _protectionCodeData[];
	static const uint8 _protectionPal[];

	Cutscene _cut;
	Menu _menu;
	Mixer _mix;
	ModPlayer _modPly;
	Resource _res;
	SfxPlayer _sfxPly;
	Video _vid;
	SystemStub *_stub;
	const char *_savePath;
	uint32_t _cheats;

	const uint8 *_stringsTable;
	const char **_textsTable;
	uint8 _currentLevel;
	uint8 _skillLevel;
	int _demoBin;	
	uint32 _score;
	uint8 _currentRoom;
	uint8 _currentIcon;
	bool _loadMap;
	uint8 _printLevelCodeCounter;
	uint32 _randSeed;
	uint16 _currentInventoryIconNum;
	uint16 _curMonsterFrame;
	uint16 _curMonsterNum;
	uint8 _blinkingConradCounter;
	uint16 _textToDisplay;
	bool _eraseBackground;
	AnimBufferState _animBuffer0State[41];
	AnimBufferState _animBuffer1State[6]; // Conrad
	AnimBufferState _animBuffer2State[42];
	AnimBufferState _animBuffer3State[12];
	AnimBuffers _animBuffers;
	uint8 _bankData[0x7000];
	uint8 *_firstBankData;
	uint8 *_lastBankData;
	BankSlot _bankSlots[49];
	BankSlot *_curBankSlot;
	const uint8 *_bankDataPtrs;
	uint16 _deathCutsceneCounter;
	bool _saveStateCompleted;
	bool _endLoop;
	uint32_t _frameTimestamp;
	uint32_t _saveTimestamp;
	
	Game(SystemStub *, const char *dataPath, const char *savePath, ResourceType ver);

	void run();
	void displayTitleScreenMac(int num);	
	void resetGameState();
	void mainLoop();
	void updateTiming();
	void playCutscene(int id = -1);
	bool playCutsceneSeq(const char *name);
	bool hasLevelMap(int level, int room) const;
	void loadLevelMap();
	void loadLevelData();
	void drawIcon(uint8_t iconNum, int16_t x, int16_t y, uint8_t colMask);
	void drawCurrentInventoryItem();
	void printLevelCode();
	void showFinalScore();
	bool handleConfigPanel();
	bool handleContinueAbort();
	bool handleProtectionScreen();
	void printSaveStateCompleted();
	void drawLevelTexts();
	void drawStoryTexts();
	void drawString(const uint8_t *p, int x, int y, uint8_t color, bool hcenter = false);
	void prepareAnims();
	void prepareAnimsHelper(LivePGE *pge, int16_t dx, int16_t dy);
	void drawAnims();
	void drawAnimBuffer(uint8_t stateNum, AnimBufferState *state);
	void drawPiege(AnimBufferState *state);
	void drawObject(const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags);
	void drawObjectFrame(const uint8_t *bankDataPtr, const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t flags);
	void drawCharacter(const uint8_t *dataPtr, int16_t x, int16_t y, uint8_t a, uint8_t b, uint8_t flags);
	int loadMonsterSprites(LivePGE *pge);
	void playSound(uint8 sfxId, uint8 softVol);
	uint16_t getRandomNumber();
	void changeLevel();
	void handleInventory();
	uint8 *findBankData(uint16 entryNum);
	void clearSaveSlots(uint8 level);

	// pieges
	bool _pge_playAnimSound;
	MessagePGE _pge_messages[256];
	MessagePGE *_pge_messagesTable[256]; // indexed by pge number
	MessagePGE *_pge_nextFreeMessage;
	LivePGE *_pge_liveTable2[256]; // active pieges list (index = pge number)
	LivePGE *_pge_liveTable1[256]; // pieges list by room (index = room)
	LivePGE _pgeLive[256];
	uint8 _pge_currentPiegeRoom;
	bool _pge_currentPiegeFacingDir; // (false == left)
	bool _pge_processOBJ;
	uint8 _pge_inpKeysMask;
	uint16_t _pge_opGunVar;
	uint16_t _pge_compareVar1;
	uint16_t _pge_compareVar2;
	uint8_t _pge_zoomPiegeNum;
	uint8_t _pge_zoomCounter;
	int _pge_zoomX, _pge_zoomY;

	void pge_resetMessages();
	void pge_clearMessages(uint8_t pge_index);
	int pge_hasMessageData(LivePGE *pge, uint16_t msg_num, uint16_t counter) const;
	void pge_loadForCurrentLevel(uint16_t idx);
	void pge_process(LivePGE *pge);
	void pge_setupNextAnimFrame(LivePGE *pge, MessagePGE *le);
	void pge_playAnimSound(LivePGE *pge, uint16_t arg2);
	void pge_setupAnim(LivePGE *pge);
	int pge_execute(LivePGE *live_pge, InitPGE *init_pge, const Object *obj);
	void pge_prepare();
	void pge_setupDefaultAnim(LivePGE *pge);
	uint16_t pge_processOBJ(LivePGE *pge);
	void pge_setupOtherPieges(LivePGE *pge, InitPGE *init_pge);
	void pge_addToCurrentRoomList(LivePGE *pge, uint8_t room);
	void pge_getInput();
	int pge_op_isInpUp(ObjectOpcodeArgs *args);
	int pge_op_isInpBackward(ObjectOpcodeArgs *args);
	int pge_op_isInpDown(ObjectOpcodeArgs *args);
	int pge_op_isInpForward(ObjectOpcodeArgs *args);
	int pge_op_isInpUpMod(ObjectOpcodeArgs *args);
	int pge_op_isInpBackwardMod(ObjectOpcodeArgs *args);
	int pge_op_isInpDownMod(ObjectOpcodeArgs *args);
	int pge_op_isInpForwardMod(ObjectOpcodeArgs *args);
	int pge_op_isInpIdle(ObjectOpcodeArgs *args);
	int pge_op_isInpNoMod(ObjectOpcodeArgs *args);
	int pge_op_getCollision0u(ObjectOpcodeArgs *args);
	int pge_op_getCollision00(ObjectOpcodeArgs *args);
	int pge_op_getCollision0d(ObjectOpcodeArgs *args);
	int pge_op_getCollision1u(ObjectOpcodeArgs *args);
	int pge_op_getCollision10(ObjectOpcodeArgs *args);
	int pge_op_getCollision1d(ObjectOpcodeArgs *args);
	int pge_op_getCollision2u(ObjectOpcodeArgs *args);
	int pge_op_getCollision20(ObjectOpcodeArgs *args);
	int pge_op_getCollision2d(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide0u(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide00(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide0d(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide1u(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide10(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide1d(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide2u(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide20(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide2d(ObjectOpcodeArgs *args);
	int pge_op_collides0o0d(ObjectOpcodeArgs *args);
	int pge_op_collides2o2d(ObjectOpcodeArgs *args);
	int pge_op_collides0o0u(ObjectOpcodeArgs *args);
	int pge_op_collides2o2u(ObjectOpcodeArgs *args);
	int pge_op_collides2u2o(ObjectOpcodeArgs *args);
	int pge_hasPiegeSentMessage(ObjectOpcodeArgs *args);
	int pge_op_sendMessageData0(ObjectOpcodeArgs *args);
	int pge_op_sendMessageData1(ObjectOpcodeArgs *args);
	int pge_op_sendMessageData2(ObjectOpcodeArgs *args);
	int pge_op_sendMessageData3(ObjectOpcodeArgs *args);
	int pge_op_isPiegeDead(ObjectOpcodeArgs *args);
	int pge_op_collides1u2o(ObjectOpcodeArgs *args);
	int pge_op_collides1u1o(ObjectOpcodeArgs *args);
	int pge_op_collides1o1u(ObjectOpcodeArgs *args);
	int pge_o_unk0x2B(ObjectOpcodeArgs *args);
	int pge_o_unk0x2C(ObjectOpcodeArgs *args);
	int pge_o_unk0x2D(ObjectOpcodeArgs *args);
	int pge_op_nop(ObjectOpcodeArgs *args);
	int pge_op_pickupObject(ObjectOpcodeArgs *args);
	int pge_op_addItemToInventory(ObjectOpcodeArgs *args);
	int pge_op_copyPiege(ObjectOpcodeArgs *args);
	int pge_op_canUseCurrentInventoryItem(ObjectOpcodeArgs *args);
	int pge_op_removeItemFromInventory(ObjectOpcodeArgs *args);
	int pge_o_unk0x34(ObjectOpcodeArgs *args);
	int pge_op_isInpMod(ObjectOpcodeArgs *args);
	int pge_op_setCollisionState1(ObjectOpcodeArgs *args);
	int pge_op_setCollisionState0(ObjectOpcodeArgs *args);
	int pge_hasMessageData0(ObjectOpcodeArgs *args);
	int pge_hasMessageData1(ObjectOpcodeArgs *args);
	int pge_hasMessageData2(ObjectOpcodeArgs *args);
	int pge_hasMessageData3(ObjectOpcodeArgs *args);
	int pge_o_unk0x3C(ObjectOpcodeArgs *args);
	int pge_o_unk0x3D(ObjectOpcodeArgs *args);
	int pge_op_setPiegeCounter(ObjectOpcodeArgs *args);
	int pge_op_decPiegeCounter(ObjectOpcodeArgs *args);
	int pge_o_unk0x40(ObjectOpcodeArgs *args);
	int pge_op_wakeUpPiege(ObjectOpcodeArgs *args);
	int pge_op_removePiege(ObjectOpcodeArgs *args);
	int pge_op_removePiegeIfNotNear(ObjectOpcodeArgs *args);
	int pge_op_loadPiegeCounter(ObjectOpcodeArgs *args);
	int pge_o_unk0x45(ObjectOpcodeArgs *args);
	int pge_o_unk0x46(ObjectOpcodeArgs *args);
	int pge_o_unk0x47(ObjectOpcodeArgs *args);
	int pge_o_unk0x48(ObjectOpcodeArgs *args);
	int pge_o_unk0x49(ObjectOpcodeArgs *args);
	int pge_op_killInventoryPiege(ObjectOpcodeArgs *args);
	int pge_op_killPiege(ObjectOpcodeArgs *args);
	int pge_op_isInCurrentRoom(ObjectOpcodeArgs *args);
	int pge_op_isNotInCurrentRoom(ObjectOpcodeArgs *args);
	int pge_op_scrollPosY(ObjectOpcodeArgs *args);
	int pge_op_playDefaultDeathCutscene(ObjectOpcodeArgs *args);
	int pge_o_unk0x50(ObjectOpcodeArgs *args);
	int pge_o_unk0x52(ObjectOpcodeArgs *args);
	int pge_o_unk0x53(ObjectOpcodeArgs *args);
	int pge_op_isPiegeNear(ObjectOpcodeArgs *args);
	int pge_op_setLife(ObjectOpcodeArgs *args);
	int pge_op_incLife(ObjectOpcodeArgs *args);
	int pge_op_setPiegeDefaultAnim(ObjectOpcodeArgs *args);
	int pge_op_setLifeCounter(ObjectOpcodeArgs *args);
	int pge_op_decLifeCounter(ObjectOpcodeArgs *args);
	int pge_op_playCutscene(ObjectOpcodeArgs *args);
	int pge_op_compareUnkVar(ObjectOpcodeArgs *args);
	int pge_op_playDeathCutscene(ObjectOpcodeArgs *args);
	int pge_o_unk0x5D(ObjectOpcodeArgs *args);
	int pge_o_unk0x5E(ObjectOpcodeArgs *args);
	int pge_o_unk0x5F(ObjectOpcodeArgs *args);
	int pge_op_findAndCopyPiege(ObjectOpcodeArgs *args);
	int pge_op_isInRandomRange(ObjectOpcodeArgs *args);
	int pge_o_unk0x62(ObjectOpcodeArgs *args);
	int pge_o_unk0x63(ObjectOpcodeArgs *args);
	int pge_o_unk0x64(ObjectOpcodeArgs *args);
	int pge_op_addToCredits(ObjectOpcodeArgs *args);
	int pge_op_subFromCredits(ObjectOpcodeArgs *args);
	int pge_o_unk0x67(ObjectOpcodeArgs *args);
	int pge_op_setCollisionState2(ObjectOpcodeArgs *args);
	int pge_op_saveState(ObjectOpcodeArgs *args);
	int pge_o_unk0x6A(ObjectOpcodeArgs *args);
	int pge_isToggleable(ObjectOpcodeArgs *args);
	int pge_o_unk0x6C(ObjectOpcodeArgs *args);
	int pge_op_isCollidingObject(ObjectOpcodeArgs *args);
	int pge_o_unk0x6E(ObjectOpcodeArgs *args);
	int pge_o_unk0x6F(ObjectOpcodeArgs *args);
	int pge_o_unk0x70(ObjectOpcodeArgs *args);
	int pge_o_unk0x71(ObjectOpcodeArgs *args);
	int pge_o_unk0x72(ObjectOpcodeArgs *args);
	int pge_o_unk0x73(ObjectOpcodeArgs *args);
	int pge_op_collides4u(ObjectOpcodeArgs *args);
	int pge_op_doesNotCollide4u(ObjectOpcodeArgs *args);
	int pge_op_isBelowConrad(ObjectOpcodeArgs *args);
	int pge_op_isAboveConrad(ObjectOpcodeArgs *args);
	int pge_op_isNotFacingConrad(ObjectOpcodeArgs *args);
	int pge_op_isFacingConrad(ObjectOpcodeArgs *args);
	int pge_op_collides2u1u(ObjectOpcodeArgs *args);
	int pge_op_displayText(ObjectOpcodeArgs *args);
	int pge_o_unk0x7C(ObjectOpcodeArgs *args);
	int pge_op_playSound(ObjectOpcodeArgs *args);
	int pge_o_unk0x7E(ObjectOpcodeArgs *args);
	int pge_o_unk0x7F(ObjectOpcodeArgs *args);
	int pge_op_setPiegePosX(ObjectOpcodeArgs *args);
	int pge_op_setPiegePosModX(ObjectOpcodeArgs *args);
	int pge_op_changeRoom(ObjectOpcodeArgs *args);
	int pge_op_hasInventoryItem(ObjectOpcodeArgs *args);
	int pge_op_changeLevel(ObjectOpcodeArgs *args);
	int pge_op_shakeScreen(ObjectOpcodeArgs *args);
	int pge_o_unk0x86(ObjectOpcodeArgs *args);
	int pge_op_playSoundGroup(ObjectOpcodeArgs *args);
	int pge_op_adjustPos(ObjectOpcodeArgs *args);
	int pge_op_setGunVar(ObjectOpcodeArgs *args);
	int pge_op_compareGunVar(ObjectOpcodeArgs *args);
	int pge_setCurrentInventoryObject(LivePGE *pge);
	void pge_updateInventory(LivePGE *pge1, LivePGE *pge2);
	void pge_reorderInventory(LivePGE *pge);
	LivePGE *pge_getPreviousInventoryItem(LivePGE *pge, LivePGE *last_pge);
	void pge_addToInventory(LivePGE *pge1, LivePGE *pge2, LivePGE *pge3);
	int pge_updateCollisionState(LivePGE *pge, int16_t pge_dy, uint8_t value);
	int pge_ZOrder(LivePGE *pge, int16_t num, pge_ZOrderCallback compare, uint16_t unk);
	void pge_sendMessage(uint8_t src_pge_index, uint8_t dst_pge_index, int16_t num);
	void pge_removeFromInventory(LivePGE *pge1, LivePGE *pge2, LivePGE *pge3);
	int pge_ZOrderByAnimY(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderByAnimYIfType(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderIfIndex(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderByIndex(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderByObj(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderIfDifferentDirection(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderIfSameDirection(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderIfTypeAndSameDirection(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderIfTypeAndDifferentDirection(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	int pge_ZOrderByNumber(LivePGE *pge1, LivePGE *pge2, uint8_t comp, uint8_t comp2);
	void pge_updateZoom();


	// collision
	CollisionSlot _col_slots[256];
	uint8 _col_curPos;
	CollisionSlot *_col_slotsTable[256];
	CollisionSlot *_col_curSlot;
	CollisionSlot2 _col_slots2[256];
	CollisionSlot2 *_col_slots2Cur;
	CollisionSlot2 *_col_slots2Next;
	uint8 _col_activeCollisionSlots[0x30 * 3]; // left, current, right
	uint8 _col_currentLeftRoom;
	uint8 _col_currentRightRoom;
	int16 _col_currentPiegeGridPosX;
	int16 _col_currentPiegeGridPosY;

	void col_prepareRoomState();
	void col_clearState();
	LivePGE *col_findPiege(LivePGE *pge, uint16 arg2);
	int16 col_findSlot(int16 pos);
	void col_preparePiegeState(LivePGE *dst_pge);
	uint16 col_getGridPos(LivePGE *pge, int16 dx);
	int16 col_getGridData(LivePGE *pge, int16 dy, int16 dx);
	uint8 col_findCurrentCollidingObject(LivePGE *pge, uint8 n1, uint8 n2, uint8 n3, LivePGE **pge_out);
	int16 col_detectHit(LivePGE *pge, int16 arg2, int16 arg4, col_Callback1 callback1, col_Callback2 callback2, int16 argA, int16 argC);
	int col_detectHitCallback2(LivePGE *pge1, LivePGE *pge2, int16 unk1, int16 unk2);
	int col_detectHitCallback3(LivePGE *pge1, LivePGE *pge2, int16 unk1, int16 unk2);
	int col_detectHitCallback4(LivePGE *pge1, LivePGE *pge2, int16 unk1, int16 unk2);
	int col_detectHitCallback5(LivePGE *pge1, LivePGE *pge2, int16 unk1, int16 unk2);
	int col_detectHitCallback1(LivePGE *pge, int16 dy, int16 unk1, int16 unk2);
	int col_detectHitCallback6(LivePGE *pge, int16 dy, int16 unk1, int16 unk2);
	int col_detectHitCallbackHelper(LivePGE *pge, int16 unk1);
	int col_detectGunHitCallback1(LivePGE *pge, int16 arg2, int16 arg4, int16 arg6);
	int col_detectGunHitCallback2(LivePGE *pge1, LivePGE *pge2, int16 arg4, int16);
	int col_detectGunHitCallback3(LivePGE *pge1, LivePGE *pge2, int16 arg4, int16);
	int col_detectGunHit(LivePGE *pge, int16 arg2, int16 arg4, col_Callback1 callback1, col_Callback2 callback2, int16 argA, int16 argC);


	// input
	uint8 _inp_lastKeysHit;
	uint8 _inp_lastKeysHitLeftRight;
	int _inp_demPos;	
	bool _inp_replay;
	bool _inp_record;
	File *_inp_demo;

	void inp_handleSpecialKeys();
	void inp_update();


	// save/load state
	uint8 _stateSlot;
	bool _validSaveState;

	void makeGameDemoName(char *buf);
	void makeGameStateName(uint8 slot, char *buf);
	bool saveGameState(uint8 slot);
	bool loadGameState(uint8 slot);
	void saveState(SAVE_BUFFER *sbuf);
	void loadState(SAVE_BUFFER *sbuf);
};

#endif // __GAME_H__
