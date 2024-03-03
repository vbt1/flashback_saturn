

extern "C" {
#include <string.h>
#include <sl_def.h>	
#include "sat_mem_checker.h"
}

#include <assert.h>
#include <stdlib.h>

//#include "fs.h"
#include "resource_mac.h"
#include "util.h"
#include "saturn_print.h"

#undef assert
#define assert(x) if(!(x)){emu_printf("assert %s %d %s\n", __FILE__,__LINE__,__func__);}


const char *ResourceMac::FILENAME1 = "Flashbck.bin";
const char *ResourceMac::FILENAME2 = "Flashbck.rsr";

//ResourceMac::ResourceMac(const char *filePath, FileSystem *fs)
ResourceMac::ResourceMac(const char *filePath, const char *dataPath)
	: _dataOffset(0), _types(0), _entries(0), _sndIndex(-1) {
	memset(&_map, 0, sizeof(_map));
//	_f.open(filePath, "rb", fs);
//emu_printf("_f.open\n");
	_f.open(filePath, dataPath, "rb");
}

ResourceMac::~ResourceMac() {
	if (_entries) {
		for (int i = 0; i < _map.typesCount; ++i) {
			sat_free(_entries[i]);
		}
		sat_free(_entries);
	}
	sat_free(_types);
	_f.close();
}

void ResourceMac::load() {

//	emu_printf("ResourceMac::load\n");

	const uint32_t sig = _f.readUint32BE();
//	slPrintHex(sig,slLocate(10,15));

	if (sig == 0x00051607) { // AppleDouble
//		debug(DBG_INFO, "Load Macintosh data from AppleDouble");
		error("Load Macintosh data from AppleDouble");
		_f.seek(24);
		const int count = _f.readUint16BE();
		for (int i = 0; i < count; ++i) {
			const int id = _f.readUint32BE();
			const int offset = _f.readUint32BE();
			const int length = _f.readUint32BE();
			if (id == 2) { // resource fork
				loadResourceFork(offset, length);
				break;
			}
		}
	} else { // MacBinary
		//debug(DBG_INFO, "Load Macintosh data from MacBinary");
		error("Load Macintosh data from MacBinary");
		_f.seek(83);
		uint32_t dataSize = _f.readUint32BE();
		uint32_t resourceOffset = 128 + ((dataSize + 127) & ~127);
		loadResourceFork(resourceOffset, dataSize);
	}
}

void ResourceMac::loadResourceFork(uint32_t resourceOffset, uint32_t dataSize) {
//emu_printf("ResourceMac::loadResourceFork\n");
// vbt : consomme 40ko de ram	
	_f.seek(resourceOffset);
	_dataOffset = resourceOffset + _f.readUint32BE();
	uint32_t mapOffset = resourceOffset + _f.readUint32BE();

	_f.seek(mapOffset + 22);
	_f.readUint16BE();
	_map.typesOffset = _f.readUint16BE();
	_map.namesOffset = _f.readUint16BE();
	_map.typesCount = _f.readUint16BE() + 1;

	_f.seek(mapOffset + _map.typesOffset + 2);
//int xx = 0;	
//emu_printf("SAT_CALLOC: _types: %d\n", sizeof(ResourceMacType));	
	_types = (ResourceMacType *)sat_calloc(_map.typesCount, sizeof(ResourceMacType));  // taille 8 LWRAM
	for (int i = 0; i < _map.typesCount; ++i) {
		_f.read(_types[i].id, 4);
		_types[i].count = _f.readUint16BE() + 1;
		_types[i].startOffset = _f.readUint16BE();
		if (_sndIndex < 0 && memcmp(_types[i].id, "snd ", 4) == 0) {
			_sndIndex = i;
		}
	}
	_entries = (ResourceMacEntry **)sat_calloc(_map.typesCount, sizeof(ResourceMacEntry *)); // taille totale 2740 HWRAM
//	xx+=sizeof(ResourceMacEntry *);
	for (int i = 0; i < _map.typesCount; ++i) {
		_f.seek(mapOffset + _map.typesOffset + _types[i].startOffset);
		_entries[i] = (ResourceMacEntry *)sat_calloc(_types[i].count, sizeof(ResourceMacEntry));
//		xx+=sizeof(ResourceMacEntry);
		for (int j = 0; j < _types[i].count; ++j) {
			_entries[i][j].id = _f.readUint16BE();
			_entries[i][j].nameOffset = _f.readUint16BE();
			_entries[i][j].dataOffset = _f.readUint32BE() & 0x00FFFFFF;
			_f.readUint32BE();
		}
		for (int j = 0; j < _types[i].count; ++j) {
			_entries[i][j].name[0] = '\0';
			if (_entries[i][j].nameOffset != 0xFFFF) {
				_f.seek(mapOffset + _map.namesOffset + _entries[i][j].nameOffset);
				const int len = _f.readByte();
//				assert(len < kResourceMacEntryNameLength - 1);
				if(len >= kResourceMacEntryNameLength - 1)
					break;
				_f.read(_entries[i][j].name, len);
				_entries[i][j].name[len] = '\0';
//					slPrint((char *)_entries[i][j].name,slLocate(10,16));
			}
		}
	}
//emu_printf("SAT_CALLOC: _entries: %d\n", xx);	
}

const ResourceMacEntry *ResourceMac::findEntry(const char *name) const {
//emu_printf("ResourceMacEntry *ResourceMac::findEntry\n");	
	for (int type = 0; type < _map.typesCount; ++type) {
		for (int i = 0; i < _types[type].count; ++i) {
			if (strcmp(name, _entries[type][i].name) == 0) {
				return &_entries[type][i];
			}
		}
	}
	return 0;
}

