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
    uint8_t _buffer[256] __attribute__ ((aligned (4)));
    uint32_t _bufPos;
    uint32_t _bufLen;

    stdFile() : _fp(0), _bufPos(0), _bufLen(0) {}

    bool open(const char *path, const char *mode) {
        _ioErr = false;
        _fp = sat_fopen(path);
        _bufPos = 0;
        _bufLen = 0;
        return _fp != NULL;
    }

    void close() {
        if (_fp) {
            sat_fclose(_fp);
            _fp = 0;
        }
        _bufPos = 0;
        _bufLen = 0;
    }

    uint32_t size() {
        return _fp ? _fp->f_size : 0;
    }

    void seek(int32_t off) {
        if (_fp) {
            sat_fseek(_fp, off, SEEK_SET);
            _bufPos = 0;
            _bufLen = 0; // Invalidate buffer on seek
        }
    }

    void read(void *ptr, uint32_t len) {
        uint8_t *dst = (uint8_t *)ptr;
        uint32_t remaining = len;

        // Use buffered data first
        while (remaining > 0 && _bufPos < _bufLen) {
            *dst++ = _buffer[_bufPos++];
            remaining--;
        }

        // Read directly if request is large
        if (remaining >= sizeof(_buffer)) {
            if (_fp) {
                uint32_t r = sat_fread(dst, 1, remaining, _fp);
                if (r != remaining) {
                    _ioErr = true;
                }
            }
            return;
        }

        // Refill buffer for small reads
        if (_fp && remaining > 0) {
            _bufLen = sat_fread(_buffer, 1, sizeof(_buffer), _fp);
            _bufPos = 0;
            if (_bufLen == 0) {
                _ioErr = true;
                return;
            }
            uint32_t to_copy = remaining < _bufLen ? remaining : _bufLen;
            memcpy(dst, _buffer, to_copy);
            _bufPos += to_copy;
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
    memset(name, 0, 50);
    bool opened = _impl->open(filename, mode);
    snprintf(name, 49, "%s", filename);
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

void File::seek(int32_t off) {
    _impl->seek(off);
}

void File::read(void *ptr, uint32_t len) {
    _impl->read(ptr, len);
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

uint16_t File::readUint16BE() {
    uint16_t value;
    _impl->read(&value, 2);
    return value;
}

uint32_t File::readUint32BE() {
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