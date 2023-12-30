
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C"
{
	#include 	<stdio.h>	
	#include 	<string.h>	
#include <sega_mem.h>
#include <sl_def.h>

#include "sat_mem_checker.h"
}
#include "saturn_print.h"

#include "decode_mac.h"
#include "file.h"
#include "unpack.h"
#include "resource.h"
#include "resource_mac.h"
#include "game.h"

extern Uint8 vceEnabled;

Resource::Resource(const char *dataPath, ResourceType type, Language lang) {
	memset(this, 0, sizeof(Resource));
	_dataPath = dataPath;
	_type = type;
	_lang = lang;
	_aba = 0;
	_mac = 0;
	_readUint16 = (_type == kResourceTypeDOS) ? READ_LE_UINT16 : READ_BE_UINT16;
	_readUint32 = (_type == kResourceTypeDOS) ? READ_LE_UINT32 : READ_BE_UINT32;
	_scratchBuffer = (uint8_t *)std_malloc(kScratchBufferSize);
emu_printf("sat_malloc kScratchBufferSize: %d %p\n",kScratchBufferSize,_scratchBuffer);	
	if (!_scratchBuffer) {
		error("Unable to allocate temporary memory buffer");
	}
	static const int kBankDataSize = 0x7000;
	_bankData = (uint8_t *)sat_malloc(kBankDataSize);
//emu_printf("sat_malloc _bankData: %d %p\n", kBankDataSize, _bankData);	
	if (!_bankData) {
		error("Unable to allocate bank data buffer");
	}
	_bankDataTail = _bankData + kBankDataSize;
	clearBankData();
}

Resource::~Resource() {
//			emu_printf("vbt Resource::~Resource free all resources !!!!!\n");	
	clearLevelRes();
	MAC_unloadLevelData();
	sat_free(_fnt);
	sat_free(_icn);
//	sat_free(_tab);
	sat_free(_spc);
	sat_free(_spr1);
	sat_free(_scratchBuffer);
	sat_free(_cmd);
	sat_free(_pol);
	sat_free(_cine_off);
	sat_free(_cine_txt);
	for (int i = 0; i < _numSfx; ++i) {
		sat_free(_sfxList[i].data);
	}
	sat_free(_sfxList);
	sat_free(_bankData);
	delete _aba;
	delete _mac;
}


void Resource::init() {
	
//	_type = kResourceTypeMac; // vbt ajout
	
//	emu_printf("Resource::init\n");
	
	switch (_type) {
	case kResourceTypeDOS:
		break;
	case kResourceTypeMac:
		File f;
//		if (_fs->exists(ResourceMac::FILENAME1)) 
		if (f.open(ResourceMac::FILENAME1, _dataPath, "rb"))
		{
			f.close();
			_mac = new ResourceMac(ResourceMac::FILENAME1, _dataPath);
		} 
//		else if (_fs->exists(ResourceMac::FILENAME2)) 
		else if (f.open(ResourceMac::FILENAME2, _dataPath, "rb")) 
		{
			f.close();
			_mac = new ResourceMac(ResourceMac::FILENAME2, _dataPath);
		}
		_mac->load();
		break;
	}
}

void Resource::setLanguage(Language lang) {
	if (_lang != lang) {
		_lang = lang;
		// reload global language specific data files
		free_TEXT();
		load_TEXT();
		free_CINE();
		load_CINE();
	}
}

bool Resource::fileExists(const char *filename) {
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		f.close();
		return true;
	} else if (_aba) {
		return _aba->findEntry(filename) != 0;
	}
	return false;
}

void Resource::clearLevelRes() {
//emu_printf("vbt clearLevelRes\n");		
	sat_free(_tbn); _tbn = 0;
	sat_free(_mbk); _mbk = 0;
	sat_free(_pal); _pal = 0;
	sat_free(_map); _map = 0;
	sat_free(_lev); _lev = 0;
	_levNum = -1;
	sat_free(_sgd); _sgd = 0;
	sat_free(_bnq); _bnq = 0;
	sat_free(_ani); _ani = 0;
	free_OBJ();
}

void Resource::load_DEM(const char *filename) {
	sat_free(_dem); _dem = 0;
	_demLen = 0;
	File f;
	if (f.open(filename, _dataPath, "rb")) {
		_demLen = f.size();
		_dem = (uint8_t *)sat_malloc(_demLen);
		if (_dem) {
			f.read(_dem, _demLen);
		}
		f.close();
	} else if (_aba) {
		uint32_t size;
		_dem = _aba->loadEntry(filename, &size);
		if (_dem) {
			_demLen = size;
		}
	}
}

void Resource::load_FIB(const char *fileName) {
//	debug(DBG_RES, "Resource::load_FIB('%s')", fileName);
	static const uint8 fibonacciTable[] = {
		0xDE, 0xEB, 0xF3, 0xF8, 0xFB, 0xFD, 0xFE, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0D, 0x15
	};
	sprintf(_entryName, "%s.FIB", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		_numSfx = f.readUint16LE();
		_sfxList = (SoundFx *)std_malloc(_numSfx * sizeof(SoundFx));
emu_printf("sat_malloc _sfxList: %d %p\n",_numSfx * sizeof(SoundFx),_sfxList);			
		if (!_sfxList) {
			error("Unable to allocate SoundFx table");
		}
		int i;
		for (i = 0; i < _numSfx; ++i) {
			SoundFx *sfx = &_sfxList[i];
			sfx->offset = f.readUint32LE();
			sfx->len = f.readUint16LE();
			sfx->data = 0;
		}
		for (i = 0; i < _numSfx; ++i) {
			SoundFx *sfx = &_sfxList[i];
			if (sfx->len == 0) {
				continue;
			}
			f.seek(sfx->offset);
			uint8 *data = (uint8 *)sat_malloc(sfx->len * 2);
			if (!data) {
				error("Unable to allocate SoundFx data buffer");
			}
			sfx->data = data;
			uint8 c = f.readByte();
			*data++ = c;
			*data++ = c;
			uint16 sz = sfx->len - 1;
			while (sz--) {
				uint8 d = f.readByte();
				c += fibonacciTable[d >> 4];
				*data++ = c;
				c += fibonacciTable[d & 15];
				*data++ = c;
			}
			sfx->len *= 2;
		}
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
		f.close();
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_MAP_menu(const char *fileName, uint8_t *dstPtr) {
//	debug(DBG_RES, "Resource::load_MAP_menu('%s')", fileName);
	static const int kMenuMapSize = 0x3800 * 4;
	snprintf(_entryName, sizeof(_entryName), "%s.MAP", fileName);
	File f;
//			emu_printf("%s\n",_entryName);		
	if (f.open(_entryName, _dataPath, "rb")) {
		f.read(dstPtr, kMenuMapSize);
//		if (f.read(dstPtr, kMenuMapSize) != kMenuMapSize) {
//			error("Failed to read '%s'", _entryName);
//		}
//		if (f.ioErr()) {
//			error("I/O error when reading '%s'", _entryName);
//		}
		f.close();
		return;
	} else if (_aba) {
		uint32_t size = 0;
		uint8_t *dat = _aba->loadEntry(_entryName, &size);
		if (dat) {
			if (size != kMenuMapSize) {
				error("Unexpected size %d for '%s'", size, _entryName);
			}
			memcpy(dstPtr, dat, size);
			sat_free(dat);
			return;
		}
	}
	error("Cannot load '%s'", _entryName);
}

void Resource::load_PAL_menu(const char *fileName, uint8 *dstPtr) {
//	emu_printf("Resource::load_PAL_menu('%s')\n", fileName);
	sprintf(_entryName, "%s.PAL", fileName);
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		if (f.size() != 768) {
			error("Wrong file size for '%s', %d", _entryName, f.size());
		}
		f.read(dstPtr, 768);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
		f.close();
	} else {
		error("Can't open '%s'", _entryName);
	}
}

void Resource::load_SPR_OFF(const char *fileName, uint8_t *sprData) {
//	debug(DBG_RES, "Resource::load_SPR_OFF('%s')", fileName);
	snprintf(_entryName, sizeof(_entryName), "%s.OFF", fileName);
	uint8_t *offData = 0;
	File f;
	if (f.open(_entryName, _dataPath, "rb")) {
		const int len = f.size();
		offData = (uint8_t *)sat_malloc(len);
		if (!offData) {
			error("Unable to allocate sprite offsets");
		}
		f.read(offData, len);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
		f.close();
	} else if (_aba) {
		offData = _aba->loadEntry(_entryName);
	}
	if (offData) {
		const uint8_t *p = offData;
		uint16_t pos;
		while ((pos = READ_LE_UINT16(p)) != 0xFFFF) {
			assert(pos < NUM_SPRITES);
			uint32_t off = READ_LE_UINT32(p + 2);
			if (off == 0xFFFFFFFF) {
				_sprData[pos] = 0;
			} else {
				_sprData[pos] = sprData + off;
			}
			p += 6;
		}
		sat_free(offData);
		return;
	}
	error("Cannot load '%s'", _entryName);
}

static const char *getCineName(Language lang, ResourceType type) {
	switch (lang) {
	case LANG_FR:
		return "FR_";
/*	case LANG_DE:
		return "GER";
	case LANG_SP:
		return "SPA";
	case LANG_IT:
		return "ITA";*/
	case LANG_EN:
	default:
		return "ENG";
	}
}

void Resource::load_CINE() {
	const char *prefix = getCineName(_lang, _type);
//	debug(DBG_RES, "Resource::load_CINE('%s')", prefix);
	File f;
			
	switch (_type) {
	case kResourceTypeDOS:
		if (_cine_off == 0) {
			snprintf(_entryName, sizeof(_entryName), "%sCINE.BIN", prefix);
			if (!f.open(_entryName, _dataPath, "rb")) {
				strcpy(_entryName, "ENGCINE.BIN");
			}
			if (f.open(_entryName, _dataPath, "rb")) {
				int len = f.size();
				_cine_off = (uint8_t *)sat_malloc(len);
				if (!_cine_off) {
					error("Unable to allocate cinematics offsets");
				}
				f.read(_cine_off, len);
				if (f.ioErr()) {
					error("I/O error when reading '%s'", _entryName);
				}
				f.close();
			} else if (_aba) {
				_cine_off = _aba->loadEntry(_entryName);
			}
		}
		if (!_cine_off) {
			error("Cannot load '%s'", _entryName);
		}
		if (_cine_txt == 0) {
			snprintf(_entryName, sizeof(_entryName), "%sCINE.TXT", prefix);
			if (!f.open(_entryName, _dataPath, "rb")) {
				strcpy(_entryName, "ENGCINE.TXT");
			}
			File f;
			if (f.open(_entryName, _dataPath, "rb")) {
				int len = f.size();
				_cine_txt = (uint8_t *)sat_malloc(len);
				if (!_cine_txt) {
					error("Unable to allocate cinematics text data");
				}
				f.read(_cine_txt, len);
				if (f.ioErr()) {
					error("I/O error when reading '%s'", _entryName);
				}
				f.close();
			} else if (_aba) {
				_cine_txt = _aba->loadEntry(_entryName);
			}
		}
		if (!_cine_txt) {
			error("Cannot load '%s'", _entryName);
		}
		break;
	case kResourceTypeMac:
		MAC_loadCutsceneText();
		break;
	}
}

void Resource::free_CINE() {
	sat_free(_cine_off);
	_cine_off = 0;
//	sat_free(_cine_txt); // vbt est dans scratchbuff
	_cine_txt = 0;
}

void Resource::load_TEXT() {
	File f;
	// Load game strings
	_stringsTable = 0;
	switch (_lang) {
	case LANG_FR:
		_stringsTable = LocaleData::_stringsTableFR;
		break;
	case LANG_EN:
		_stringsTable = LocaleData::_stringsTableEN;
		break;/*
	case LANG_DE:
		_stringsTable = LocaleData::_stringsTableDE;
		break;
	case LANG_SP:
		_stringsTable = LocaleData::_stringsTableSP;
		break;
	case LANG_IT:
		_stringsTable = LocaleData::_stringsTableIT;
		break;
	case LANG_JP:
		_stringsTable = LocaleData::_stringsTableJP;
		break;*/
	}
	// Load menu strings
	_textsTable = 0;
	switch (_lang) {
	case LANG_FR:
		_textsTable = LocaleData::_textsTableFR;
		break;
	case LANG_EN:
		_textsTable = LocaleData::_textsTableEN;
		break;/*
	case LANG_DE:
		_textsTable = LocaleData::_textsTableDE;
		break;
	case LANG_SP:
		_textsTable = LocaleData::_textsTableSP;
		break;
	case LANG_IT:
		_textsTable = LocaleData::_textsTableIT;
		break;
	case LANG_JP:
		_textsTable = LocaleData::_textsTableEN;
		break;*/
	}
}

void Resource::free_TEXT() {
	if (_extTextsTable) {
		sat_free(_extTextsTable);
		_extTextsTable = 0;
	}
	_stringsTable = 0;
	if (_extStringsTable) {
		sat_free(_extStringsTable);
		_extStringsTable = 0;
	}
	_textsTable = 0;
}

static const char *getTextBin(Language lang, ResourceType type) {
	// FB PC-CD version has language specific files
	// .TBN is used as fallback if open fails
	switch (lang) {
	case LANG_FR:
		return "TBF";
/*	case LANG_DE:
		return "TBG";
	case LANG_SP:
		return "TBS";
	case LANG_IT:
		return "TBI";*/
	case LANG_EN:
	default:
		return "TBN";
	}
}

void Resource::unload(int objType) {
	switch (objType) {
	case OT_CMD:
		sat_free(_cmd);
		_cmd = 0;
		break;
	case OT_POL:
		sat_free(_pol);
		_pol = 0;
		break;
	case OT_CMP:
		sat_free(_cmd);
		_cmd = 0;
		sat_free(_pol);
		_pol = 0;
		break;
	default:
		error("Unimplemented Resource::unload() type %d", objType);
		break;
	}
}

void Resource::load(const char *objName, int objType, const char *ext) {
	emu_printf("Resource::load('%s', %d)\n", objName, objType);
	LoadStub loadStub = 0;
	File f;
		
	switch (objType) {
	case OT_MBK:
		snprintf(_entryName, sizeof(_entryName), "%s.MBK", objName);
		loadStub = &Resource::load_MBK;
		break;
	case OT_PGE:
		snprintf(_entryName, sizeof(_entryName), "%s.PGE", objName);
		loadStub = &Resource::load_PGE;
		break;
	case OT_PAL:
		snprintf(_entryName, sizeof(_entryName), "%s.PAL", objName);
		loadStub = &Resource::load_PAL;
		break;
	case OT_CT:
		snprintf(_entryName, sizeof(_entryName), "%s.CT", objName);
		loadStub = &Resource::load_CT;
		break;
	case OT_MAP:
		snprintf(_entryName, sizeof(_entryName), "%s.MAP", objName);
		loadStub = &Resource::load_MAP;
		break;
	case OT_SPC:
		snprintf(_entryName, sizeof(_entryName), "%s.SPC", objName);
		loadStub = &Resource::load_SPC;
		break;
	case OT_RP:
		snprintf(_entryName, sizeof(_entryName), "%s.RP", objName);
		loadStub = &Resource::load_RP;
		break;
	case OT_RPC:
		snprintf(_entryName, sizeof(_entryName), "%s.RPC", objName);
		loadStub = &Resource::load_RP;
		break;
	case OT_SPR:
		snprintf(_entryName, sizeof(_entryName), "%s.SPR", objName);
		loadStub = &Resource::load_SPR;
		break;
	case OT_SPRM:
		snprintf(_entryName, sizeof(_entryName), "%s.SPR", objName);
		loadStub = &Resource::load_SPRM;
		break;
	case OT_ICN:
		snprintf(_entryName, sizeof(_entryName), "%s.ICN", objName);
//		emu_printf("%s.ICN\n", objName);
		loadStub = &Resource::load_ICN;
		break;
	case OT_FNT:
		snprintf(_entryName, sizeof(_entryName), "%s.FNT", objName);
		loadStub = &Resource::load_FNT;
		break;
	case OT_OBJ:
		snprintf(_entryName, sizeof(_entryName), "%s.OBJ", objName);
		loadStub = &Resource::load_OBJ;
		break;
	case OT_ANI:
		snprintf(_entryName, sizeof(_entryName), "%s.ANI", objName);
		loadStub = &Resource::load_ANI;
		break;
	case OT_TBN:
		snprintf(_entryName, sizeof(_entryName), "%s.%s", objName, getTextBin(_lang, _type));
//		if (!_fs->exists(_entryName)) 

		if (!f.open(_entryName, _dataPath, "rb")) 
		{
			snprintf(_entryName, sizeof(_entryName), "%s.TBN", objName);
		}
		loadStub = &Resource::load_TBN;
		break;
	case OT_CMD:
		snprintf(_entryName, sizeof(_entryName), "%s.CMD", objName);
		loadStub = &Resource::load_CMD;
		break;
	case OT_POL:
		snprintf(_entryName, sizeof(_entryName), "%s.POL", objName);
		loadStub = &Resource::load_POL;
		break;
	case OT_CMP:
		snprintf(_entryName, sizeof(_entryName), "%s.CMP", objName);
		loadStub = &Resource::load_CMP;
		break;
	case OT_OBC:
		snprintf(_entryName, sizeof(_entryName), "%s.OBC", objName);
		loadStub = &Resource::load_OBC;
		break;
	case OT_SPL:
		snprintf(_entryName, sizeof(_entryName), "%s.SPL", objName);
		loadStub = &Resource::load_SPL;
		break;
	case OT_LEV:
		snprintf(_entryName, sizeof(_entryName), "%s.LEV", objName);
		loadStub = &Resource::load_LEV;
		break;
	case OT_SGD:
		snprintf(_entryName, sizeof(_entryName), "%s.SGD", objName);
		loadStub = &Resource::load_SGD;
		break;
	case OT_BNQ:
		snprintf(_entryName, sizeof(_entryName), "%s.BNQ", objName);
		loadStub = &Resource::load_BNQ;
		break;
	case OT_SPM:
		snprintf(_entryName, sizeof(_entryName), "%s.SPM", objName);
		loadStub = &Resource::load_SPM;
		break;
	default:
		error("Unimplemented Resource::load() type %d", objType);
		break;
	}
	if (ext) {
		snprintf(_entryName, sizeof(_entryName), "%s.%s", objName, ext);
	}

	if (f.open(_entryName, _dataPath, "rb")) {
		assert(loadStub);
		(this->*loadStub)(&f);
		if (f.ioErr()) {
			error("I/O error when reading '%s'", _entryName);
		}
		f.close();
	} else {
		if (_aba) {
			uint32_t size;
			uint8_t *dat = _aba->loadEntry(_entryName, &size);
			if (dat) {
				switch (objType) {
				case OT_MBK:
					_mbk = dat;
					break;
				case OT_PGE:
					decodePGE(dat, size);
					break;
				case OT_PAL:
					_pal = dat;
					break;
				case OT_CT:
					if (!bytekiller_unpack((uint8_t *)_ctData, sizeof(_ctData), dat, size)) {
						error("Bad CRC for '%s'", _entryName);
					}
					sat_free(dat);
					break;
				case OT_SPC:
					_spc = dat;
					_numSpc = READ_BE_UINT16(_spc) / 2;
					break;
				case OT_RP:
					if (size != sizeof(_rp)) {
						error("Unexpected size %d for '%s'", size, _entryName);
					}
					memcpy(_rp, dat, size);
					sat_free(dat);
					break;
				case OT_ICN:
					_icn = dat;
					break;
				case OT_FNT:
					_fnt = dat;
					break;
				case OT_OBJ:
					_numObjectNodes = READ_LE_UINT16(dat);
					assert(_numObjectNodes == 230);
					decodeOBJ(dat + 2, size - 2);
					break;
				case OT_ANI:
					_ani = dat;
					break;
				case OT_TBN:
					_tbn = dat;
					break;
				case OT_CMD:
					_cmd = dat;
					break;
				case OT_POL:
					_pol = dat;
					break;
				case OT_SGD:
					_sgd = dat;
					_sgd[0] = 0; // clear number of entries, fix first offset
					break;
				case OT_BNQ:
					_bnq = dat;
					break;
				default:
					error("Cannot load '%s' type %d", _entryName, objType);
				}
				return;
			}
		}
		error("Cannot open '%s'", _entryName);
	}
	f.close();
}
void Resource::load_CT(File *pf) {
//	debug(DBG_RES, "Resource::load_CT()");
	int len = pf->size();
	uint8_t *tmp = (uint8 *)sat_malloc(len);
	if (!tmp) {
		error("Unable to allocate CT buffer");
	} else {
		pf->read(tmp, len);
		if (!bytekiller_unpack((uint8_t *)_ctData, sizeof(_ctData), tmp, len)) {
			error("Bad CRC for collision data");
		}
		sat_free(tmp);
	}
}

void Resource::load_FNT(File *f) {
//	debug(DBG_RES, "Resource::load_FNT()");
	int len = f->size();
	_fnt = (uint8 *)sat_malloc(len);
	if (!_fnt) {
		error("Unable to allocate FNT buffer");
	} else {
		f->read(_fnt, len);
	}
}

void Resource::load_MBK(File *f) {
//	debug(DBG_RES, "Resource::load_MBK()");
	int len = f->size();
	_mbk = (uint8_t *)sat_malloc(len);
	if (!_mbk) {
		error("Unable to allocate MBK buffer");
	} else {
		f->read(_mbk, len);
	}
}

void Resource::load_ICN(File *f) {
//	debug(DBG_RES, "Resource::load_ICN()");
	int len = f->size();
	if (_icnLen == 0) {
		if(_icn==NULL)
			_icn = (uint8_t *)sat_malloc(len);
	} else {
		_icn = (uint8_t *)sat_realloc(_icn, _icnLen + len);
	}
	if (!_icn) {
		error("Unable to allocate ICN buffer");
	} else {
		f->read(_icn + _icnLen, len);
	}
	_icnLen += len;
}

void Resource::load_SPR(File *f) {
//	debug(DBG_RES, "Resource::load_SPR()");
	int len = f->size() - 12;
	_spr1 = (uint8 *)sat_malloc(len);
	if (!_spr1) {
		error("Unable to allocate SPR1 buffer");
	} else {
		f->seek(12);
		f->read(_spr1, len);
	}
}

void Resource::load_SPRM(File *f) {
//	debug(DBG_RES, "Resource::load_SPRM()");
	const uint32_t len = f->size() - 12;
	assert(len <= sizeof(_sprm));
	f->seek(12);
	f->read(_sprm, len);
}

void Resource::load_RP(File *f) {
//	debug(DBG_RES, "Resource::load_RP()");
	f->read(_rp, sizeof(_rp));
}

void Resource::load_SPC(File *f) {
//	debug(DBG_RES, "Resource::load_SPC()");
	int len = f->size();
	_spc = (uint8 *)sat_malloc(len);
	if (!_spc) {
		error("Unable to allocate SPC buffer");
	} else {
		f->read(_spc, len);
		_numSpc = READ_BE_UINT16(_spc) / 2;
	}
}

void Resource::load_PAL(File *f) {
//	debug(DBG_RES, "Resource::load_PAL()");
	int len = f->size();
	_pal = (uint8 *)sat_malloc(len);
	if (!_pal) {
		error("Unable to allocate PAL buffer");
	} else {
		f->read(_pal, len);
	}
}

void Resource::load_MAP(File *f) {
//	debug(DBG_RES, "Resource::load_MAP()");

#ifdef _RAMCART_
	int len = f->size();
	_map = (uint8_t *)sat_malloc(len);
	if (!_map) {
		error("Unable to allocate MAP buffer");
	} else {
		f->read(_map, len);
	}
#endif
}

void Resource::load_OBJ(File *f) {
//	debug(DBG_RES, "Resource::load_OBJ()");

	_numObjectNodes = f->readUint16LE();
	assert(_numObjectNodes < 255);
	uint32_t offsets[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = f->readUint32LE();
	}
	offsets[_numObjectNodes] = f->size() - 2;
	int numObjectsCount = 0;
	uint16_t objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
//			debug(DBG_RES, "i=%d objectsCount[numObjectsCount]=%d", i, objectsCount[numObjectsCount]);
			++numObjectsCount;
		}
	}
	uint32_t prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)sat_malloc(sizeof(ObjectNode));
			if (!on) {
				error("Unable to allocate ObjectNode num=%d", i);
			}
			f->seek(offsets[i] + 2);
			on->num_objects = f->readUint16LE();
//			debug(DBG_RES, "count=%d", on->num_objects, objectsCount[iObj]);
			assert(on->num_objects == objectsCount[iObj]);
			on->objects = (Object *)sat_malloc(sizeof(Object) * on->num_objects);
			for (int j = 0; j < on->num_objects; ++j) {
				Object *obj = &on->objects[j];
				obj->type = f->readUint16LE();
				obj->dx = f->readByte();
				obj->dy = f->readByte();
				obj->init_obj_type = f->readUint16LE();
				obj->opcode2 = f->readByte();
				obj->opcode1 = f->readByte();
				obj->flags = f->readByte();
				obj->opcode3 = f->readByte();
				obj->init_obj_number = f->readUint16LE();
				obj->opcode_arg1 = f->readUint16LE();
				obj->opcode_arg2 = f->readUint16LE();
				obj->opcode_arg3 = f->readUint16LE();
//				debug(DBG_RES, "obj_node=%d obj=%d op1=0x%X op2=0x%X op3=0x%X", i, j, obj->opcode2, obj->opcode1, obj->opcode3);
			}
			++iObj;
			prevOffset = offsets[i];
			prevNode = on;
		}
		_objectNodesMap[i] = prevNode;
	}
}

void Resource::free_OBJ() {
	emu_printf("Resource::free_OBJ()\n");
	ObjectNode *prevNode = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (_objectNodesMap[i] != prevNode) {
			ObjectNode *curNode = _objectNodesMap[i];
			sat_free(curNode->objects);
			sat_free(curNode);
			prevNode = curNode;
		}
		_objectNodesMap[i] = 0;
	}
}

void Resource::load_OBC(File *f) {
	const int packedSize = f->readUint32BE();
	uint8_t *packedData = (uint8_t *)sat_malloc(packedSize);
emu_printf("load_OBC %p %d\n", packedData,packedSize);	
	if (!packedData) {
		error("Unable to allocate OBC temporary buffer 1");
	}
	f->seek(packedSize);
	const int unpackedSize = f->readUint32BE();
	uint8_t *tmp = (uint8_t *)sat_malloc(unpackedSize);
	if (!tmp) {
		error("Unable to allocate OBC temporary buffer 2");
	}
	f->seek(4);
	f->read(packedData, packedSize);
	if (!bytekiller_unpack(tmp, unpackedSize, packedData, packedSize)) {
		error("Bad CRC for compressed object data");
	}
	
	sat_free(packedData);
	decodeOBJ(tmp, unpackedSize);
	sat_free(tmp);
}

void Resource::decodeOBJ(const uint8_t *tmp, int size) {
	uint32_t offsets[256];
	int tmpOffset = 0;
	_numObjectNodes = 230;
	if (_type == kResourceTypeMac) {
		_numObjectNodes = _readUint16(tmp);
		tmpOffset += 2;
	}
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = _readUint32(tmp + tmpOffset); tmpOffset += 4;
	}
	offsets[_numObjectNodes] = size;
	int numObjectsCount = 0;
	uint16_t objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
			++numObjectsCount;
		}
	}
	uint32_t prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)sat_malloc(sizeof(ObjectNode));
			if (!on) {
				error("Unable to allocate ObjectNode num=%d", i);
			}
			const uint8_t *objData = tmp + offsets[i];
			on->num_objects = _readUint16(objData); objData += 2;
			assert(on->num_objects == objectsCount[iObj]);
			on->objects = (Object *)sat_malloc(sizeof(Object) * on->num_objects);
			for (int j = 0; j < on->num_objects; ++j) {
				Object *obj = &on->objects[j];
				obj->type = _readUint16(objData); objData += 2;
				obj->dx = *objData++;
				obj->dy = *objData++;
				obj->init_obj_type = _readUint16(objData); objData += 2;
				obj->opcode2 = *objData++;
				obj->opcode1 = *objData++;
				obj->flags = *objData++;
				obj->opcode3 = *objData++;
				obj->init_obj_number = _readUint16(objData); objData += 2;
				obj->opcode_arg1 = _readUint16(objData); objData += 2;
				obj->opcode_arg2 = _readUint16(objData); objData += 2;
				obj->opcode_arg3 = _readUint16(objData); objData += 2;
//				debug(DBG_RES, "obj_node=%d obj=%d op1=0x%X op2=0x%X op3=0x%X", i, j, obj->opcode2, obj->opcode1, obj->opcode3);
			}
			++iObj;
			prevOffset = offsets[i];
			prevNode = on;
		}
		_objectNodesMap[i] = prevNode;
	}
}

void Resource::load_PGE(File *f) {
//	debug(DBG_RES, "Resource::load_PGE()");
	_pgeNum = f->readUint16LE();
	memset(_pgeInit, 0, sizeof(_pgeInit));
//	debug(DBG_RES, "_pgeNum=%d", _pgeNum);
	assert(_pgeNum <= ARRAYSIZE(_pgeInit));
	for (uint16_t i = 0; i < _pgeNum; ++i) {
		InitPGE *pge = &_pgeInit[i];
		pge->type = f->readUint16LE();
		pge->pos_x = f->readUint16LE();
		pge->pos_y = f->readUint16LE();
		pge->obj_node_number = f->readUint16LE();
		pge->life = f->readUint16LE();
		for (int lc = 0; lc < 4; ++lc) {
			pge->data[lc] = f->readUint16LE();
		}
		pge->object_type = f->readByte();
		pge->init_room = f->readByte();
		pge->room_location = f->readByte();
		pge->init_flags = f->readByte();
		pge->colliding_icon_num = f->readByte();
		pge->icon_num = f->readByte();
		pge->object_id = f->readByte();
		pge->skill = f->readByte();
		pge->mirror_x = f->readByte();
		pge->flags = f->readByte();
		pge->collision_data_len = f->readByte();
		f->readByte();
		pge->text_num = f->readUint16LE();
	}
}

void Resource::decodePGE(const uint8_t *p, int size) {
	_pgeNum = _readUint16(p); 
	p += 2;
	memset(_pgeInit, 0, sizeof(_pgeInit));
	assert(_pgeNum <= ARRAYSIZE(_pgeInit));
	for (uint16_t i = 0; i < _pgeNum; ++i) {
		InitPGE *pge = &_pgeInit[i];
		pge->type = _readUint16(p); p += 2;
		pge->pos_x = _readUint16(p); p += 2;
		pge->pos_y = _readUint16(p); p += 2;
		pge->obj_node_number = _readUint16(p); p += 2;
		pge->life = _readUint16(p); p += 2;
		for (int lc = 0; lc < 4; ++lc) {
			pge->data[lc] = _readUint16(p); p += 2;
		}
		pge->object_type = *p++;
		pge->init_room = *p++;
		pge->room_location = *p++;
		pge->init_flags = *p++;
		pge->colliding_icon_num = *p++;
		pge->icon_num = *p++;
		pge->object_id = *p++;
		pge->skill = *p++;
		pge->mirror_x = *p++;
		pge->flags = *p++;
		pge->collision_data_len = *p++;
		++p;
		pge->text_num = _readUint16(p); p += 2;
	}
}

void Resource::load_ANI(File *f) {
//	debug(DBG_RES, "Resource::load_ANI()");
	const int size = f->size();
	_ani = (uint8_t *)std_malloc(size);
	if (!_ani) {
		error("Unable to allocate ANI buffer");
	} else {
		f->read(_ani, size);
	}
}

void Resource::load_TBN(File *f) {
	emu_printf("Resource::load_TBN()\n");
	int len = f->size();
	_tbn = (uint8_t *)sat_malloc(len);
	if (!_tbn) {
		error("Unable to allocate TBN buffer");
	} else {
		f->read(_tbn, len);
	}
}

void Resource::load_CMD(File *pf) {
//	debug(DBG_RES, "Resource::load_CMD()");
	sat_free(_cmd);
	int len = pf->size();
	_cmd = (uint8_t *)sat_malloc(len);
	if (!_cmd) {
		error("Unable to allocate CMD buffer");
	} else {
		pf->read(_cmd, len);
	}
}

void Resource::load_POL(File *pf) {
//	debug(DBG_RES, "Resource::load_POL()");
	sat_free(_pol);
	int len = pf->size();
	_pol = (uint8_t *)sat_malloc(len);
	if (!_pol) {
		error("Unable to allocate POL buffer");
	} else {
		pf->read(_pol, len);
	}
}

void Resource::load_CMP(File *pf) {
	sat_free(_pol);
	sat_free(_cmd);
	int len = pf->size();
	uint8_t *tmp = (uint8_t *)sat_malloc(len);
	if (!tmp) {
		error("Unable to allocate CMP buffer");
	}
	pf->read(tmp, len);
	struct {
		int offset, packedSize, size;
	} data[2];
	int offset = 0;
	for (int i = 0; i < 2; ++i) {
		int packedSize = READ_BE_UINT32(tmp + offset); offset += 4;
		assert((packedSize & 1) == 0);
		if (packedSize < 0) {
			data[i].size = packedSize = -packedSize;
		} else {
			data[i].size = READ_BE_UINT32(tmp + offset + packedSize - 4);
		}
		data[i].offset = offset;
		data[i].packedSize = packedSize;
		offset += packedSize;
	}
	_pol = (uint8_t *)sat_malloc(data[0].size);
	if (!_pol) {
		error("Unable to allocate POL buffer");
	}
	if (data[0].packedSize == data[0].size) {
		memcpy(_pol, tmp + data[0].offset, data[0].packedSize);
	} else if (!bytekiller_unpack(_pol, data[0].size, tmp + data[0].offset, data[0].packedSize)) {
		error("Bad CRC for cutscene polygon data");
	}
	_cmd = (uint8_t *)sat_malloc(data[1].size);
	if (!_cmd) {
		error("Unable to allocate CMD buffer");
	}
	if (data[1].packedSize == data[1].size) {
		memcpy(_cmd, tmp + data[1].offset, data[1].packedSize);
	} else if (!bytekiller_unpack(_cmd, data[1].size, tmp + data[1].offset, data[1].packedSize)) {
		error("Bad CRC for cutscene command data");
	}
	sat_free(tmp);
}

void Resource::load_VCE(int num, int segment, uint8_t **buf, uint32_t *bufSize) {
	*buf = 0;
	int offset = _voicesOffsetsTable[num];
	if (offset != 0xFFFF) {
		const uint16_t *p = _voicesOffsetsTable + offset / 2;
		offset = (*p++) * 2048;
		int count = *p++;
		if (segment < count) {
			File f;
			if (vceEnabled && f.open("VOICE.VCE", _dataPath, "rb")) {
				int voiceSize = p[segment] * 2048 / 5;
				uint8_t *voiceBuf = (uint8_t *)sat_malloc(voiceSize);
				if (voiceBuf) {
					uint8_t *dst = voiceBuf;
					offset += 0x2000;
					for (int s = 0; s < count; ++s) {
						int len = p[s] * 2048;
						for (int i = 0; i < len / (0x2000 + 2048); ++i) {
							if (s == segment) {
								f.seek(offset);
								int n = 2048;
								while (n--) {
									int v = f.readByte();
									if (v & 0x80) {
										v = -(v & 0x7F);
									}
									*dst++ = (uint8_t)(v & 0xFF);
								}
							}
							offset += 0x2000 + 2048;
						}
						if (s == segment) {
							break;
						}
					}
					*buf = voiceBuf;
					*bufSize = voiceSize;
				}
			}
		}
	}
}

static void normalizeSPL(SoundFx *sfx) {
	static const int kGain = 2;

	sfx->peak = ABS(sfx->data[0]);
	for (int i = 1; i < sfx->len; ++i) {
		const int8_t sample = sfx->data[i];
		if (ABS(sample) > sfx->peak) {
			sfx->peak = ABS(sample);
		}
		sfx->data[i] = sample / kGain;
	}
}

void Resource::load_SPL(File *f) {
	for (int i = 0; i < _numSfx; ++i) {
		sat_free(_sfxList[i].data);
	}
	sat_free(_sfxList);
	_numSfx = NUM_SFXS;
	_sfxList = (SoundFx *)std_calloc(_numSfx, sizeof(SoundFx));
	if (!_sfxList) {
		error("Unable to allocate SoundFx table");
	}
	int offset = 0;
	for (int i = 0; i < _numSfx; ++i) {
		const int size = f->readUint16BE(); offset += 2;
		if ((size & 0x8000) != 0) {
			continue;
		}
		debug(DBG_RES, "sfx=%d size=%d", i, size);
		assert(size != 0 && (size & 1) == 0);
		if (i == 64) {
			warning("Skipping sound #%d (%s) size %d", i, _splNames[i], size);
			f->seek(offset + size);
		}  else {
			_sfxList[i].offset = offset;
			_sfxList[i].freq = kPaulaFreq / 650;
			_sfxList[i].data = (uint8_t *)sat_malloc(size);
			if (_sfxList[i].data) {
				f->read(_sfxList[i].data, size);
				_sfxList[i].len = size;
				normalizeSPL(&_sfxList[i]);
			}
		}
		offset += size;
	}
}

void Resource::load_LEV(File *f) {
emu_printf("load_LEV %d\n", f->size());	
	const int len = f->size();
	_lev = (uint8_t *)sat_malloc(len);
	if (!_lev) {
		error("Unable to allocate LEV buffer");
	} else {
		f->read(_lev, len);
	}
}

void Resource::load_SGD(File *f) {
	const int len = f->size();
	if (_type == kResourceTypeDOS) {
		_sgd = (uint8_t *)sat_malloc(len);
		if (!_sgd) {
			error("Unable to allocate SGD buffer");
		} else {
			f->read(_sgd, len);
			// first byte == number of entries, clear to fix up 32 bits offset
			_sgd[0] = 0;
		}
		return;
	}
	f->seek(len - 4);
	int size = f->readUint32BE();
	f->seek(0);
	uint8_t *tmp = (uint8_t *)sat_malloc(len);
	if (!tmp) {
		error("Unable to allocate SGD temporary buffer");
	}
	f->read(tmp, len);
	_sgd = (uint8_t *)sat_malloc(size);
	if (!_sgd) {
		error("Unable to allocate SGD buffer");
	}
	if (!bytekiller_unpack(_sgd, size, tmp, len)) {
		error("Bad CRC for SGD data");
	}
	sat_free(tmp);
}

void Resource::load_BNQ(File *f) {
	const int len = f->size();
	_bnq = (uint8_t *)sat_malloc(len);
	if (!_bnq) {
		error("Unable to allocate BNQ buffer");
	} else {
		f->read(_bnq, len);
	}
}

void Resource::load_SPM(File *f) {
	static const int kPersoDatSize = 178647;
	const int len = f->size();
	f->seek(len - 4);
	const uint32_t size = f->readUint32BE();
	f->seek(0);
	uint8_t *tmp = (uint8_t *)sat_malloc(len);
	if (!tmp) {
		error("Unable to allocate SPM temporary buffer");
	}
	f->read(tmp, len);
	if (size == kPersoDatSize) {
		_spr1 = (uint8_t *)sat_malloc(size);
		if (!_spr1) {
			error("Unable to allocate SPR1 buffer");
		}
		if (!bytekiller_unpack(_spr1, size, tmp, len)) {
			error("Bad CRC for SPM data");
		}
	} else {
		assert(size <= sizeof(_sprm));
		if (!bytekiller_unpack(_sprm, sizeof(_sprm), tmp, len)) {
			error("Bad CRC for SPM data");
		}
	}
	for (int i = 0; i < NUM_SPRITES; ++i) {
		const uint32_t offset = _spmOffsetsTable[i];
		if (offset >= kPersoDatSize) {
			_sprData[i] = _sprm + offset - kPersoDatSize;
		} else {
			_sprData[i] = _spr1 + offset;
		}
	}
	sat_free(tmp);
}

void Resource::clearBankData() {
	_bankBuffersCount = 0;
	_bankDataHead = _bankData;
}

int Resource::getBankDataSize(uint16_t num) {
	int len = READ_BE_UINT16(_mbk + num * 6 + 4);
	switch (_type) {
	case kResourceTypeDOS:
		if (len & 0x8000) {
			if (_mbk == _bnq) { // demo .bnq use signed int
				len = -(int16_t)len;
				break;
			}
			len &= 0x7FFF;
		}
		break;
	case kResourceTypeMac:
		assert(0); // different graphics format
		break;
	}
	return len * 32;
}

uint8_t *Resource::findBankData(uint16_t num) {
	for (int i = 0; i < _bankBuffersCount; ++i) {
		if (_bankBuffers[i].entryNum == num) {
			return _bankBuffers[i].ptr;
		}
	}
	return 0;
}

uint8_t *Resource::loadBankData(uint16_t num) {
	const uint8_t *ptr = _mbk + num * 6;
	int dataOffset = READ_BE_UINT32(ptr);
	if (_type == kResourceTypeDOS) {
		// first byte of the data buffer corresponds
		// to the total count of entries
		dataOffset &= 0xFFFF;
	}
	const int size = getBankDataSize(num);
	if (size == 0) {
		warning("Invalid bank data %d", num);
		return _bankDataHead;
	}
	const int avail = _bankDataTail - _bankDataHead;
	if (avail < size) {
		clearBankData();
	}
	assert(_bankDataHead + size <= _bankDataTail);
	assert(_bankBuffersCount < (int)ARRAYSIZE(_bankBuffers));
	_bankBuffers[_bankBuffersCount].entryNum = num;
	_bankBuffers[_bankBuffersCount].ptr = _bankDataHead;
	const uint8_t *data = _mbk + dataOffset;
	if (READ_BE_UINT16(ptr + 4) & 0x8000) {
		memcpy(_bankDataHead, data, size);
	} else {
		assert(dataOffset > 4);
		assert(size == (int)READ_BE_UINT32(data - 4));
		if (!bytekiller_unpack(_bankDataHead, _bankDataTail - _bankDataHead, data, 0)) {
			error("Bad CRC for bank data %d", num);
		}
	}
	uint8_t *bankData = _bankDataHead;
	_bankDataHead += size;
	return bankData;
}

uint8_t *Resource::decodeResourceMacText(const char *name, const char *suffix) {
//		emu_printf("decodeResourceMacText        \n");
	char buf[256];
	snprintf(buf, sizeof(buf), "%s %s", name, suffix);
	const ResourceMacEntry *entry = _mac->findEntry(buf);
	if (entry) {
		return decodeResourceMacData(entry, false);
	} else { // CD version
		if (strcmp(name, "Flashback") == 0) {
			name = "Game";
		}
		const char *language = (_lang == LANG_FR) ? "French" : "English";
		snprintf(buf, sizeof(buf), "%s %s %s", name, suffix, language);
		return decodeResourceMacData(buf, false);
	}
}

uint8_t *Resource::decodeResourceMacData(const char *name, bool decompressLzss) {
	uint8_t *data = 0;
//		emu_printf("decodeResourceMacData 1       \n");	
	const ResourceMacEntry *entry = _mac->findEntry(name);
	if (entry) {
//		emu_printf("Resource '%s' found %d %s\n",name, decompressLzss,entry->name);		
		data = decodeResourceMacData(entry, decompressLzss);
	} else {
		_resourceMacDataSize = 0;
		emu_printf("Resource '%s' not found\n", name);
	}
	return data;
}

uint8_t *Resource::decodeResourceMacData(const ResourceMacEntry *entry, bool decompressLzss) {
//	emu_printf("Resource::decodeResourceMacData '%d'\n",entry->dataOffset);	
//	assert(entry);
	_mac->_f.seek(_mac->_dataOffset + entry->dataOffset);
	_resourceMacDataSize = _mac->_f.readUint32BE();
emu_printf("entry->name %s lzss %d size %d\n",entry->name, decompressLzss, _resourceMacDataSize);
	uint8_t *data = 0;
	if (decompressLzss) {
emu_printf("decodeLzss %d %s\n",_resourceMacDataSize, entry->name);		
		data = decodeLzss(_mac->_f, entry->name, _scratchBuffer, _resourceMacDataSize);
		if (!data) {
			emu_printf("Failed to decompress '%s'\n", entry->name);
		}
	} else {

		if(strcmp("Flashback strings", entry->name) == 0)
		{
//emu_printf("vbt Flashback strings!!!!\n");
			data = (uint8_t *)sat_malloc(_resourceMacDataSize);
//			data = (uint8_t *)_scratchBuffer+0x12C00;
		}
		else if(strstr(entry->name,"names") !=NULL)
		{
//emu_printf("vbt aaaaaaaaa Flashback strings std_malloc!!!!\n");				
			data = (uint8_t *)sat_malloc(_resourceMacDataSize);
		}		
		else if(strcmp("Flashback colors", entry->name) == 0 
		|| strcmp("Flashback colors", entry->name) == 0 
		|| strncmp("Title", entry->name, 5) == 0  
		|| strncmp("intro", entry->name, 5) == 0 
		|| strncmp("Movie", entry->name, 5) == 0 
		|| strncmp("logo", entry->name, 4)
//		|| strcmp("Flashback strings", entry->name) == 0
		)
		{
//emu_printf("ducon _scratchBuffer %d\n", _resourceMacDataSize);	
			data = (uint8_t *)_scratchBuffer; //+0x12C00;//std_malloc(_resourceMacDataSize);
		}
				
		else
		{
			data = (uint8_t *)sat_malloc(_resourceMacDataSize);
		}		
		
		if (!data) {
			emu_printf("Failed to allocate %d bytes for '%s'\n", _resourceMacDataSize, entry->name);
		} else {
//			emu_printf("_mac->_f.read(data, _resourceMacDataSize %d\n",_resourceMacDataSize);
			_mac->_f.read(data, _resourceMacDataSize);
		}
	}
//emu_printf("end Resource::decodeResourceMacData %d %s\n",_resourceMacDataSize,entry->name);	
	return data;
}

void Resource::MAC_decodeImageData(const uint8_t *ptr, int i, DecodeBuffer *dst) {
	
	const uint8_t *basePtr = ptr;
	const uint16_t sig = READ_BE_UINT16(ptr); ptr += 2;
//	assert(sig == 0xC211 || sig == 0xC103);
	if(sig != 0xC211 && sig != 0xC103)
		return;
	const int count = READ_BE_UINT16(ptr); ptr += 2;
//	assert(i < count);
	if(i>=count)
		return;
	
	ptr += 4;
	const uint32_t offset = READ_BE_UINT32(ptr + i * 4);
	if (offset != 0) {
		ptr = basePtr + offset;
		const int w = READ_BE_UINT16(ptr); ptr += 2;
		const int h = READ_BE_UINT16(ptr); ptr += 2;
		switch (sig) {
		case 0xC211:
			decodeC211(ptr + 4, w, h, dst);
			break;
		case 0xC103:
			decodeC103(ptr, w, h, dst);
			break;
		}
	}
}

void Resource::MAC_decodeDataCLUT(const uint8_t *ptr) {
	ptr += 6; // seed+flags
	_clutSize = READ_BE_UINT16(ptr); ptr += 2;
	
	assert(_clutSize < kClutSize);
	for (int i = 0; i < _clutSize; ++i) {
		const int index = READ_BE_UINT16(ptr); ptr += 2;
		assert(i == index);
		// ignore lower bits
		_clut[i].r = (ptr[0] & 0xff) >> 2; ptr += 2;
		_clut[i].g = (ptr[0] & 0xff) >> 2; ptr += 2;
		_clut[i].b = (ptr[0] & 0xff) >> 2; ptr += 2;
	
/*	
		_clut[i].r = (ptr[0] & 0xF00) >> 6; ptr += 2;
		_clut[i].g = (ptr[0] & 0x0F0) >> 2; ptr += 2;
		_clut[i].b = (ptr[0] & 0x00F) << 2; ptr += 2;
		
		_clut[i].r = ptr[0]; ptr += 2;
		_clut[i].g = ptr[0]; ptr += 2;
		_clut[i].b = ptr[0]; ptr += 2;
			c.r = ((color & 0xF00) >> 6);
			c.g = ((color & 0x0F0) >> 2) | t;
			c.b = ((color & 0x00F) << 2) | t;		
*/		
	}
}

void Resource::MAC_loadClutData() {
//emu_printf("MAC_loadClutData\n");		
	uint8_t *ptr = decodeResourceMacData("Flashback colors", false);
	MAC_decodeDataCLUT(ptr);
//	sat_free(ptr);
}

void Resource::MAC_loadFontData() {
//emu_printf("MAC_loadFontData\n");	
	_fnt = decodeResourceMacData("Font", true);   // taille 19323 hwr
}

void Resource::MAC_loadIconData() {
//emu_printf("MAC_loadIconData\n");			
	_icn = decodeResourceMacData("Icons", true);
}

void Resource::MAC_loadPersoData() {
//emu_printf("MAC_loadPersoData\n");				
	_perso = decodeResourceMacData("Person", true); // taille 213124 lwr
}

void Resource::MAC_loadMonsterData(const char *name, Color *clut) {
emu_printf("MAC_loadMonsterData\n");						
	static const struct {
		const char *id;
		const char *name;
		int index;
	} data[] = {
		{ "junky", "Junky", 0x32 },
		{ "mercenai", "Mercenary", 0x34 },
		{ "replican", "Replicant", 0x35 },
		{ "glue", "Alien", 0x36 },
		{ 0, 0, 0 }
	};
	sat_free(_monster);
	_monster = 0;
	for (int i = 0; data[i].id; ++i) {
		if (strcmp(data[i].id, name) == 0) {
			_monster = decodeResourceMacData(data[i].name, true);
			if(_monster==NULL)
			{
emu_printf("%s not loaded\n",data[i].name);				
				return;
			}
			emu_printf("MAC_loadMonsterData %s %p \n",name,_monster);
			MAC_copyClut16(clut, 5, data[i].index);
			break;
		}
	}
}

void Resource::MAC_loadTitleImage(int i, DecodeBuffer *buf) {
//emu_printf("MAC_loadTitleImage\n");	
	char name[64];
	snprintf(name, sizeof(name), "Title %d", i);
//emu_printf("decodeResourceMacData %s\n",name);	
	uint8_t *ptr = decodeResourceMacData(name, (i == 6));
	if (ptr) {
//emu_printf("MAC_decodeImageData\n");		
		MAC_decodeImageData(ptr, 0, buf);
//emu_printf("end MAC_decodeImageData\n");
//		sat_free(ptr);  // pas de vidage car on utilise scratchbuffer
	}
}

void Resource::MAC_unloadLevelData() {
	emu_printf("unload _ani %p\n",_ani);	
	sat_free(_ani); // vbt est dans scratchbuff
	emu_printf("unload _ani %p\n",_ani);	
	_ani = 0;
	ObjectNode *prevNode = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevNode != _objectNodesMap[i]) {
	emu_printf("unload _objectNodesMap[%d] %p\n",i,_objectNodesMap[i]);			
			sat_free(_objectNodesMap[i]);
			prevNode = _objectNodesMap[i];
		}
	}
	_numObjectNodes = 0;
	sat_free(_tbn);
	_tbn = 0;
	sat_free(_str);
	_str = 0;
// vbt ajout	
//	sat_free(_spc);
//	_spc = 0;	
}

static const int _macLevelColorOffsets[] = { 24, 28, 36, 40, 44 }; // red palette: 32
static const char *_macLevelNumbers[] = { "1", "2", "3", "4-1", "4-2", "5-1", "5-2" };

void Resource::MAC_loadLevelData(int level) {
	char name[64];
//emu_printf("MAC_loadLevelData\n");	
	// .PGE
	snprintf(name, sizeof(name), "Level %s objects", _macLevelNumbers[level]);
	uint8_t *ptr = decodeResourceMacData(name, true);
emu_printf("decodePGE\n");		
	decodePGE(ptr, _resourceMacDataSize);
	sat_free(ptr);
emu_printf(" .ANI\n");	
	// .ANI
	snprintf(name, sizeof(name), "Level %s sequences", _macLevelNumbers[level]);
	_ani = decodeResourceMacData(name, true);
	assert(READ_BE_UINT16(_ani) == 0x48D);
emu_printf(" .OBJ\n");
	// .OBJ
	snprintf(name, sizeof(name), "Level %s conditions", _macLevelNumbers[level]);
	ptr = decodeResourceMacData(name, true);
	assert(READ_BE_UINT16(ptr) == 0xE6);
	decodeOBJ(ptr, _resourceMacDataSize);
	sat_free(ptr);
emu_printf(" .CT\n");
	// .CT
	snprintf(name, sizeof(name), "Level %c map", _macLevelNumbers[level][0]);
	ptr = decodeResourceMacData(name, true);
	assert(_resourceMacDataSize == 0x1D00);
	memcpy(_ctData, ptr, _resourceMacDataSize);
	sat_free(ptr);
//emu_printf(" .SPC\n");
	// .SPC
	snprintf(name, sizeof(name), "Objects %c", _macLevelNumbers[level][0]);
	_spc = decodeResourceMacData(name, true);
//emu_printf(" .TBN\n");
	// .TBN
	snprintf(name, sizeof(name), "Level %s", _macLevelNumbers[level]);
	_tbn = decodeResourceMacText(name, "names");
//emu_printf(" .Flashback text _tbn %p\n",_tbn);
	_str = decodeResourceMacText("Flashback", "strings");
//emu_printf(" .Flashback strings _str %p\n",_str);	
}

void Resource::MAC_loadLevelRoom(int level, int i, DecodeBuffer *dst) {
//emu_printf("MAC_loadLevelRoom\n");	
	char name[64];
	snprintf(name, sizeof(name), "Level %c Room %d", _macLevelNumbers[level][0], i);
	uint8_t *ptr = decodeResourceMacData(name, true);
	MAC_decodeImageData(ptr, 0, dst);
	sat_free(ptr);
}

void Resource::MAC_clearClut16(Color *clut, uint8_t dest) {
        memset(&clut[dest * 16], 0, 16 * sizeof(Color));
}

void Resource::MAC_copyClut16(Color *clut, uint8_t dest, uint8_t src) {
	memcpy(&clut[dest * 16], &_clut[src * 16], 16 * sizeof(Color));
}	

void Resource::MAC_setupRoomClut(int level, int room, Color *clut) {
	const int num = _macLevelNumbers[level][0] - '1';
	int offset = _macLevelColorOffsets[num];
	if (level == 1) {
		switch (room) {
		case 27:
		case 28:
		case 29:
		case 30:
		case 35:
		case 36:
		case 37:
		case 45:
		case 46:
			offset = 32;
			break;
		}
	}
	for (int i = 0; i < 4; ++i) {
		MAC_copyClut16(clut, i, offset + i);
		MAC_copyClut16(clut, 8 + i, offset + i);
	}
	MAC_copyClut16(clut, 4, 0x30);
	// 5 is monster palette
	MAC_clearClut16(clut, 6);
	MAC_copyClut16(clut, 0xA, _macLevelColorOffsets[0] + 2);
	MAC_copyClut16(clut, 0xC, 0x37);
	MAC_copyClut16(clut, 0xD, 0x38);
}

const uint8_t *Resource::MAC_getImageData(const uint8_t *ptr, int i) {
	const uint8_t *basePtr = ptr;
	const uint16_t sig = READ_BE_UINT16(ptr); 
	ptr += 2;
	if(sig != 0xC211)
		return NULL;
	const int count = READ_BE_UINT16(ptr);
	ptr += 2;
	
	if (!(i < count))
		return NULL;		
	ptr += 4;
	const uint32_t offset = READ_BE_UINT32(ptr + i * 4);
	return (offset != 0) ? basePtr + offset : 0;
}

bool Resource::MAC_hasLevelMap(int level, int room) const {
	char name[64];
	snprintf(name, sizeof(name), "Level %c Room %d", _macLevelNumbers[level][0], room);
	return _mac->findEntry(name) != 0;
}

static void stringLowerCase(char *p) {
	while (*p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
		++p;
	}
}

void Resource::MAC_unloadCutscene() {
//	emu_printf("MAC_unloadCutscene\n");	
	sat_free(_cmd);
	_cmd = 0;
	sat_free(_pol);
	_pol = 0;
}

void Resource::MAC_loadCutscene(const char *cutscene) {
//	emu_printf("MAC_loadCutscene\n");
	MAC_unloadCutscene();
	char name[32];

	snprintf(name, sizeof(name), "%s movie", cutscene);
	stringLowerCase(name);
	const ResourceMacEntry *cmdEntry = _mac->findEntry(name);
	if (!cmdEntry) {
		return;
	}
	_cmd = decodeResourceMacData(cmdEntry, true);

	snprintf(name, sizeof(name), "%s polygons", cutscene);
	stringLowerCase(name);
	const ResourceMacEntry *polEntry = _mac->findEntry(name);
	if (!polEntry) {
		return;
	}
	_pol = decodeResourceMacData(polEntry, true);
}

void Resource::MAC_loadCutsceneText() {
	_cine_txt = decodeResourceMacText("Movie", "strings");
	_cine_off = 0; // offsets are prepended to _cine_txt
}

void Resource::MAC_loadCreditsText() {
	_credits = decodeResourceMacText("Credit", "strings");
}

void Resource::MAC_loadSounds() {
	static const int8_t table[NUM_SFXS] = {
		0, -1,  1,  2,  3,  4, -1,  5,  6,  7,  8,  9, 10, 11, -1, 12,
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, 26, 27,
		28, -1, 29, -1, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
		42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, -1, 53, 54, 55, 56,
		-1, 57
	};
	_numSfx = NUM_SFXS;
	_sfxList = (SoundFx *)std_calloc(_numSfx, sizeof(SoundFx));
	if (!_sfxList) {
		return;
	}
	static const int kHeaderSize = 0x24;
	const int soundType = _mac->_sndIndex;
	assert(soundType != -1);
	for (int i = 0; i < NUM_SFXS; ++i) {
		const int num = table[i];
		if (num != -1) {
			assert(num >= 0 && num < _mac->_types[soundType].count);
			const ResourceMacEntry *entry = &_mac->_entries[soundType][num];
			_mac->_f.seek(_mac->_dataOffset + entry->dataOffset);
			int dataSize = _mac->_f.readUint32BE();
			assert(dataSize > kHeaderSize);
			uint8_t buf[kHeaderSize];
			_mac->_f.read(buf, kHeaderSize);
			dataSize -= kHeaderSize;
			uint8_t *p = (uint8_t *)sat_malloc(dataSize);
			if (p) {
				_mac->_f.read(p, dataSize);
				for (int j = 0; j < dataSize; ++j) {
					p[j] ^= 0x80;
				}
				_sfxList[i].len = READ_BE_UINT32(buf + 0x12);
				_sfxList[i].freq = READ_BE_UINT16(buf + 0x16);
				_sfxList[i].data = p;
				debug(DBG_RES, "sfx #%d len %d datasize %d freq %d", i, _sfxList[i].len, dataSize, _sfxList[i].freq);
			}
		}
	}
}

