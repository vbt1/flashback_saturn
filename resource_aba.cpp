extern "C" {
#include "sat_mem_checker.h"
#include <stdio.h>
#include <string.h>
}
#include "resource_aba.h"
#include "unpack.h"
#include "util.h"
#include "saturn_print.h"

#undef assert
#define assert(x) if(!(x)){emu_printf("assert %s %d %s\n", __FILE__,__LINE__,__func__);}

int	 strcasecmp(const char *, const char *) __pure;
char *_dataPath = "";	
	
ResourceAba::ResourceAba(FileSystem *fs)
	: _fs(fs) {
		
	_dataPath = "";
	_filesCount = 0;
	_entries = 0;
	_entriesCount = 0;
}

ResourceAba::~ResourceAba() {
	sat_free(_entries);
}

static int compareAbaEntry(const void *a, const void *b) {
	return strcasecmp(((const ResourceAbaEntry *)a)->name, ((const ResourceAbaEntry *)b)->name);
}

void ResourceAba::readEntries(const char *aba) {
	assert(_filesCount < 3);
	if (_f[_filesCount].open(aba, _dataPath, "rb")) {
//	if (_f[_filesCount].open(aba, "rb", _fs)) {
		File &f = _f[_filesCount];
		const int currentCount = _entriesCount;
		const int entriesCount = f.readUint16BE();
		_entries = (ResourceAbaEntry *)sat_realloc(_entries, (currentCount + entriesCount) * sizeof(ResourceAbaEntry));
		if (!_entries) {
			error("Failed to allocate %d _entries", currentCount + entriesCount);
			return;
		}
		_entriesCount = currentCount + entriesCount;
		const int entrySize = f.readUint16BE();
		assert(entrySize == 30);
		uint32_t nextOffset = 0;
		for (int i = 0; i < entriesCount; ++i) {
			const int j = currentCount + i;
			f.read(_entries[j].name, sizeof(_entries[j].name));
			_entries[j].offset = f.readUint32BE();
			_entries[j].compressedSize = f.readUint32BE();
			_entries[j].size = f.readUint32BE();
			_entries[j].fileIndex = _filesCount;
			const uint32_t tag = f.readUint32BE();
			assert(tag == TAG);
//			debug(DBG_RES, "'%s' offset 0x%X size %d/%d", _entries[j].name,  _entries[j].offset, _entries[j].compressedSize, _entries[j].size);
			if (i != 0) {
				assert(nextOffset == _entries[j].offset);
			}
			nextOffset = _entries[j].offset + _entries[j].compressedSize;
		}
//		qsort(_entries, _entriesCount, sizeof(ResourceAbaEntry), compareAbaEntry);
		++_filesCount;
	}
}

const ResourceAbaEntry *ResourceAba::findEntry(const char *name) const {
	ResourceAbaEntry tmp;
	strcpy(tmp.name, name);
	return &tmp; //(const ResourceAbaEntry *)bsearch(&tmp, _entries, _entriesCount, sizeof(ResourceAbaEntry), compareAbaEntry);
}

uint8_t *ResourceAba::loadEntry(const char *name, uint32_t *size) {
	uint8_t *dst = 0;
	const ResourceAbaEntry *e = findEntry(name);
	if (e) {
		if (size) {
			*size = e->size;
		}
		
		uint8_t *tmp = (uint8_t *)sat_malloc(e->compressedSize);
emu_printf("ResourceAba::loadEntry sat_malloc:  %p %d\n",e->compressedSize,tmp);			

		if (!tmp) {
//			emu_printf("Failed to allocate %d bytes\n", e->compressedSize);
			return 0;
		}
		_f[e->fileIndex].seek(e->offset);
		_f[e->fileIndex].read(tmp, e->compressedSize);
		if (e->compressedSize == e->size) {
			dst = tmp;
		} else {
			dst = (uint8_t *)sat_malloc(e->size);
			if (!dst) {
//				emu_printf("Failed to allocate %d bytes\n", e->size);
				sat_free(tmp);
				return 0;
			}
			const bool ret = bytekiller_unpack(dst, e->size, tmp, e->compressedSize);
			if (!ret) {
//				emu_printf("Bad CRC for '%s'\n", name);
			}
emu_printf("ResourceAba::loadEntry: sat_free  %p\n",tmp);			
			sat_free(tmp);
		}
	}
	return dst;
}
