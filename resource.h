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

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "intern.h"
#include "resource_aba.h"
#include "resource_mac.h"

struct DecodeBuffer;
struct File;
//struct FileSystem;

struct LocaleData {
	enum Id {
		LI_01_CONTINUE_OR_ABORT = 0,
		LI_02_TIME,
		LI_03_CONTINUE,
		LI_04_ABORT,
		LI_05_COMPLETED,
		LI_06_LEVEL,
		LI_07_START,
		LI_08_SKILL,
		LI_09_PASSWORD,
		LI_10_INFO,
		LI_11_QUIT,
		LI_12_SKILL_LEVEL,
		LI_13_EASY,
		LI_14_NORMAL,
		LI_15_EXPERT,
		LI_16_ENTER_PASSWORD1,
		LI_17_ENTER_PASSWORD2,
		LI_18_RESUME_GAME,
		LI_19_ABORT_GAME,
		LI_20_LOAD_GAME,
		LI_21_SAVE_GAME,
		LI_22_SAVE_SLOT,

		LI_NUM
	};

	static const char *_textsTableFR[];
	static const char *_textsTableEN[];
	static const char *_textsTableDE[];
	static const char *_textsTableSP[];
	static const uint8_t _stringsTableFR[];
	static const uint8_t _stringsTableEN[];
	static const uint8_t _stringsTableDE[];
	static const uint8_t _stringsTableSP[];
};

struct Resource {
	typedef void (Resource::*LoadStub)(File *);

	enum ObjectType {
		OT_MBK,
		OT_PGE,
		OT_PAL,
		OT_CT,
		OT_MAP,
		OT_SPC,
		OT_RP,
		OT_RPC,
		OT_DEMO,
		OT_ANI,
		OT_OBJ,
		OT_TBN,
		OT_SPR,
		OT_TAB,
		OT_ICN,
		OT_FNT,
		OT_TXTBIN,
		OT_CMD,
		OT_POL,
		OT_SPRM,
		OT_OFF,
		OT_CMP,
		OT_OBC,
		OT_SPL,
		OT_LEV,
		OT_SGD,
		OT_BNQ,
		OT_SPM
	};


	enum {
		NUM_SFXS = 66,
		NUM_BANK_BUFFERS = 50,
		NUM_CUTSCENE_TEXTS = 117,
		NUM_SPRITES = 1287
	};

	enum {
		kPaulaFreq = 3546897,
		kClutSize = 1024,
		kScratchBufferSize = 320 * 224 + 1024
	};

	static const uint16_t _voicesOffsetsTable[];
	static const uint32_t _spmOffsetsTable[];
	static const char *_splNames[];
	static const uint8_t _gameSavedSoundData[];
	static const uint16_t _gameSavedSoundLen;

	ResourceMac *_mac;
	ResourceAba *_aba;
	Language _lang;
	uint16_t (*_readUint16)(const void *);
	uint32_t (*_readUint32)(const void *);	
	const char *_dataPath;
	ResourceType _type;
	char _entryName[30];
	uint8_t *_fnt;
	uint8_t *_mbk;
	uint8_t *_icn;
	int _icnLen;	
	uint8_t *_tab;
	uint8_t *_spc; // BE
	uint16 _numSpc;
	uint8_t _rp[0x4A];
	uint8_t *_pal; // BE
	uint8_t *_ani;
	uint8_t *_tbn;
	int8 _ctData[0x1D00];
	uint8_t *_spr1;
	uint8_t *_sprData[NUM_SPRITES]; // 0-0x22F + 0x28E-0x2E9 ... conrad, 0x22F-0x28D : junkie
	uint8_t _sprm[0x8411]; // MERCENAI.SPR size
	uint16 _pgeNum;
	InitPGE _pgeInit[256];
	
	uint8_t *_map;
	uint8_t *_lev;	
	char _mapFilename[50];

	uint8_t *_sgd;
	uint8_t *_bnq;
	uint16 _numObjectNodes;
	ObjectNode *_objectNodesMap[255];
	uint8_t *_memBuf;
	SoundFx *_sfxList;
	uint8_t _numSfx;
	uint8_t *_cmd;
	uint8_t *_pol;
	uint8_t *_cine_off;
	uint8_t *_cine_txt;
	uint8_t *_voiceBuf;
	char **_extTextsTable;
	const char **_textsTable;
	uint8_t *_extStringsTable;
	const uint8_t *_stringsTable;
	uint8_t *_bankData;
	uint8_t *_bankDataHead;
	uint8_t *_bankDataTail;
	BankSlot _bankBuffers[NUM_BANK_BUFFERS];
	int _bankBuffersCount;
	uint8_t *_dem;
	int _demLen;
	uint32_t _resourceMacDataSize;
	int _clutSize;
	Color _clut[kClutSize];
	uint8_t *_perso;
	uint8_t *_monster;
	uint8_t *_str;
	uint8_t *_credits;
	
	Resource(const char *dataPath, ResourceType ver);
	~Resource();

	void init();

	void clearLevelRes();
	void load_FIB(const char *fileName);
	void load_MAP_menu(const char *fileName, uint8_t *dstPtr);
	void load_PAL_menu(const char *fileName, uint8_t *dstPtr);
	void load_SPR_OFF(const char *fileName, uint8_t *sprData);
	void load_CINE();
	void load_TEXT();
	void free_TEXT();
	void unload(int objType);
	void load(const char *objName, int objType, const char *ext = 0);
	void load_CT(File *pf);
	void load_FNT(File *pf);
	void load_MBK(File *pf);
	void load_ICN(File *pf);
	void load_SPR(File *pf);
	void load_SPRM(File *pf);
	void load_RP(File *pf);
	void load_SPC(File *pf);
	void load_PAL(File *pf);
	void load_MAP(File *pf);
	void load_OBJ(File *pf);
	void free_OBJ();
	void load_OBC(File *pf);
	void decodeOBJ(const uint8_t *, int);	
	void load_PGE(File *pf);
	void decodePGE(const uint8_t *, int);	
	void load_ANI(File *pf);
	void load_TBN(File *pf);
	void load_CMD(File *pf);
	void load_POL(File *pf);
	void load_CMP(File *pf);	
	void load_VCE(int num, int segment, uint8_t **buf, uint32 *bufSize);
	void load_SPL(File *pf);
	void load_LEV(File *pf);
	void load_SGD(File *pf);
	void load_BNQ(File *pf);
	void load_SPM(File *f);	
	const uint8_t *getGameString(int num) {
		return _stringsTable + READ_LE_UINT16(_stringsTable + num * 2);
	}
	const char *getMenuString(int num) {
		return (num >= 0 && num < LocaleData::LI_NUM) ? _textsTable[num] : "";
	}
	void clearBankData();
	int getBankDataSize(uint16_t num);	
	uint8_t *findBankData(uint16_t num);
	uint8_t *loadBankData(uint16_t num);	
	
	uint8_t *decodeResourceMacText(const char *name, const char *suffix);
	uint8_t *decodeResourceMacData(const char *name, bool decompressLzss);
	uint8_t *decodeResourceMacData(const ResourceMacEntry *entry, bool decompressLzss);
	void MAC_decodeImageData(const uint8_t *ptr, int i, DecodeBuffer *dst);
	void MAC_decodeDataCLUT(const uint8_t *ptr);
	void MAC_loadClutData();
	void MAC_loadFontData();
	void MAC_loadIconData();
	void MAC_loadPersoData();
	void MAC_loadMonsterData(const char *name, Color *clut);
	void MAC_loadTitleImage(int i, DecodeBuffer *buf);
	void MAC_unloadLevelData();
	void MAC_loadLevelData(int level);
	void MAC_loadLevelRoom(int level, int i, DecodeBuffer *dst);
	void MAC_clearClut16(Color *clut, uint8_t dest);
	void MAC_copyClut16(Color *clut, uint8_t dest, uint8_t src);
	void MAC_setupRoomClut(int level, int room, Color *clut);
	const uint8_t *MAC_getImageData(const uint8_t *ptr, int i);
	bool MAC_hasLevelMap(int level, int room) const;
	void MAC_unloadCutscene();
	void MAC_loadCutscene(const char *cutscene);
	void MAC_loadCutsceneText();
	void MAC_loadCreditsText();
	void MAC_loadSounds();
};

#endif // __RESOURCE_H__
