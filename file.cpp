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
extern "C" {
#include <string.h>
#include "gfs_wrap.h"
}
#include "file.h"
#include "saturn_print.h"

struct File_impl {
	bool _ioErr;
	File_impl() : _ioErr(false) {}
	virtual bool open(const char *path, const char *mode) = 0;
	virtual void close() = 0;
	virtual uint32_t size() = 0;
	virtual void seek(int32_t off) = 0;
	virtual void read(void *ptr, uint32_t len) = 0;
	virtual void write(void *ptr, uint32_t len) = 0;
};

struct stdFile : File_impl {
	GFS_FILE *_fp;
	stdFile() : _fp(0) {}
	bool open(const char *path, const char *mode) {
		_ioErr = false;
		_fp = sat_fopen(path);
		return (_fp != NULL);
	}
	void close() {
		if (_fp) {
			sat_fclose(_fp);
			_fp = 0;
		}
	}
	uint32_t size() {
		uint32_t sz = 0;
		if (_fp) {
			sz = _fp->f_size;
		}
		return sz;
	}
	void seek(int32_t off) {
		if (_fp) {
			sat_fseek(_fp, off, SEEK_SET);
		}
	}
	void read(void *ptr, uint32_t len) {
		if (_fp) {
			uint32_t r = sat_fread(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}
	void write(void *ptr, uint32_t len) {
		if (_fp) {
			uint32_t r = 0;
			if (r != len) {
				_ioErr = true;
			}
		}
	}
};

File::File() {
	_impl = new stdFile;
}

File::~File() {
	if (_impl) {
		_impl->close();
		delete _impl;
	}
}

bool File::open(const char *filename, const char *directory, const char *mode) {
	_impl->close();
//emu_printf("File::open(%s)\n",filename);	
	memset(name, 0, 50);
//	char buf[512];
//	sprintf(buf, "%s/%s", directory, filename);
//	char *p = buf + strlen(directory) + 1;
//	string_lower(p);
	bool opened = _impl->open(filename, mode);
	snprintf(name, 49, "%s", filename);	
//	if (!opened) { // let's try uppercase
//		string_upper(p);
//		opened = _impl->open(buf, mode);
//	}
	return opened;
}

void File::close() {
emu_printf("File::close()\n");	
	if (_impl) {
		_impl->close();
	}
}

bool File::ioErr() const {
	return _impl->_ioErr;
}

uint32_t File::size() {
	return _impl->size();
}

void File::seek(int32_t off) {
	_impl->seek(off);
}

void File::read(void *ptr, uint32_t len) {
//emu_printf("File::read %p\n",ptr);		
	_impl->read(ptr, len);
//emu_printf("File::read done\n");
}

uint8_t File::readByte() {
	uint8_t b;
	read(&b, 1);
	return b;
}

uint16_t File::readUint16LE() {
	uint8_t lo = readByte();
	uint8_t hi = readByte();
	return (hi << 8) | lo;
}

uint32_t File::readUint32LE() {
	uint16_t lo = readUint16LE();
	uint16_t hi = readUint16LE();
	return (hi << 16) | lo;
}

uint16_t File::readUint16BE() {
	uint8_t hi = readByte();
	uint8_t lo = readByte();
	return (hi << 8) | lo;
}

uint32_t File::readUint32BE() {
	uint16_t hi = readUint16BE();
	uint16_t lo = readUint16BE();
	return (hi << 16) | lo;
}

void File::write(void *ptr, uint32_t len) {
	_impl->write(ptr, len);
}

void File::writeByte(uint8_t b) {
	write(&b, 1);
}

void File::writeUint16BE(uint16_t n) {
	writeByte(n >> 8);
	writeByte(n & 0xFF);
}

void File::writeUint32BE(uint32_t n) {
	writeUint16BE(n >> 16);
	writeUint16BE(n & 0xFFFF);
}

char* File::fileName(void) {
	return name;
}
