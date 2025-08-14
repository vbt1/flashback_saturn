#pragma GCC optimize ("O2")

/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2007 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
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

#define GFS_BYTE_SCT(byte, sctsiz)  \
    ((Sint32)(((Uint32)(byte)) + ((Uint32)(sctsiz)) - 1) / ((Uint32)(sctsiz)))

struct File_impl {
    bool _ioErr;
    File_impl() : _ioErr(false) {}
    virtual bool open(const char *path, const int position) = 0;
    virtual void close() = 0;
    virtual uint32_t size() = 0;
    virtual uint32_t tell() = 0;
    virtual void seek(int32_t off) = 0;
	virtual void batchSeek(int32_t off);
    virtual void read(void *ptr, uint32_t len) = 0;
    virtual void write(void *ptr, uint32_t len) = 0;
	virtual uint8_t *batchRead (uint8_t *ptr, uint32_t len);
};

struct stdFile : File_impl {
    GFS_FILE *_fp;
    stdFile() : _fp(0) {}

    bool open(const char *path, const int position) {
        _ioErr = false;
        _fp = sat_fopen(path, position);
        return _fp != NULL;
    }

    void close() {
        if (_fp) {
//			emu_printf("position before closing %d\n",GFS_Tell(_fp->fid));
            sat_fclose(_fp);
            _fp = 0;
        }
    }

    uint32_t size() {
        return _fp ? _fp->f_size : 0;
    }

    uint32_t tell() {
        return _fp ? GFS_Tell(_fp->fid) : 0;
 //       return _fp->f_seek_pos;
    }

	void seek(int32_t off) {
		if (_fp) {
			sat_fseek(_fp, off, SEEK_SET);
		}
	}

	void batchSeek(int32_t off) {
		_fp->f_seek_pos += off;
	}

	void read(void *ptr, uint32_t len) {
		if (_fp) {
			uint32 r = sat_fread(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}

    void write(void *ptr, uint32_t len) {
        if (_fp) {
            uint32_t r = 0; // Adjust if writing is needed
            if (r != len) {
                _ioErr = true;
            }
        }
    }

	uint8_t *batchRead (uint8_t *ptr, uint32_t len)
	{
		Uint32 start_sector = (_fp->f_seek_pos)/SECTOR_SIZE;
	//	Uint32 skip_bytes = (stream->f_seek_pos)%SECTOR_SIZE; // Bytes to skip at the beginning of sector
		Uint32 skip_bytes = _fp->f_seek_pos & (SECTOR_SIZE - 1);
//		GFS_Seek(_fp->fid, start_sector, GFS_SEEK_SET);
		Sint32 tot_bytes = len + skip_bytes;
		Sint32 tot_sectors = GFS_BYTE_SCT(tot_bytes, SECTOR_SIZE);
		Uint32 readBytes = GFS_Fread(_fp->fid, tot_sectors, ptr, tot_bytes);
//		_fp->f_seek_pos += (readBytes - skip_bytes); 
// vbt : voir si utile, testé avec, commit sans !
		seek(readBytes - skip_bytes);

		return &ptr[skip_bytes];
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

bool File::open(const char *filename, const char *directory, const int position) {
    _impl->close();
    memset(name, 0, 20);
    bool opened = _impl->open(filename, position);
    snprintf(name, 19, "%s", filename);
    return opened;
}

void File::close() {
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

uint32_t File::tell() {
    return _impl->tell();
}

void File::seek(int32_t off) {
    _impl->seek(off);
}

void File::batchSeek(int32_t off) {
    _impl->seek(off);
}

void File::read(void *ptr, uint32_t len) {
    _impl->read(ptr, len);
}

uint8_t *File::batchRead(uint8_t *ptr, uint32_t len) {
    return _impl->batchRead(ptr, len);
}
/*
void File::readInline(void *ptr, uint32_t len) {
    _impl->read(ptr, len);
}
*/
uint8_t File::readByte() {
    uint8_t b;
    _impl->read(&b, 1);
   return b;
}
/*
uint16_t File::readUint16BE() {
	uint8_t hi = readByte();
	uint8_t lo = readByte();
	return (hi << 8) | lo;
}

uint32_t File::readUint32BE() {
	uint16_t hi = readUint16BE();
emu_printf("hi %d\n",hi);
	uint16_t lo = readUint16BE();
emu_printf("lo %d\n",lo);
	return (hi << 16) | lo;
}
*/

uint16_t File::readUint16BE() {
    uint16_t value;
    _impl->read(&value, 2);
    return value;
}

uint32_t File::readUint32BE() {
//	emu_printf("readUint32BE\n");
    uint32_t value;
    _impl->read(&value, 4);
    return value;
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

char* File::fileName() {
    return name;
}