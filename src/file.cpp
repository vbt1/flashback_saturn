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
extern GfsMng   *gfs_mng_ptr;
}
#include "file.h"
#include "saturn_print.h"
#define MNG_SVR(mng)            ((mng)->svr)
#define MNG_FILE(mng)           ((mng)->file)
#define MNG_OPENMAX(mng)        ((mng)->openmax)
#define SVR_NFILE(svr)          ((svr)->nfile)
#define GFS_FILE_USED(file)     ((file)->used)
#define GFS_FILE_TRANS(file)    ((file)->trans)
#define GFS_TRN_MODE(tran)      ((tran)->mode)
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
//    virtual void write(void *ptr, uint32_t len) = 0;
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

    GfsSvr      *svr = &MNG_SVR(gfs_mng_ptr);
    int i = SVR_NFILE(svr);

emu_printf("batchRead %d nfiles %d sz %d\n", (_fp->f_seek_pos)/SECTOR_SIZE, i, len);

//    GfsFile     *fp;	
//    fp = MNG_FILE(gfs_mng_ptr);
//    for (i = 0; i < 2; i++) {
		emu_printf("1used %d mode %d stat %d\n",     _fp->fid->used, _fp->fid->amode,_fp->fid->astat);
		emu_printf("1sct %d sctcnt %d sctmax %d\n",     _fp->fid->flow.sct, _fp->fid->flow.sctcnt,_fp->fid->flow.sctmax);
		emu_printf("1fid %d name %s mode %d stat %d\n",_fp->fid->flow.finfo.fid, GFS_IdToName(_fp->fid->flow.finfo.fid),_fp->fid->flow.finfo.sctsz,_fp->fid->flow.finfo.nsct);
		emu_printf("1id %d sect %d pos %d\n",_fp->fid, (_fp->f_seek_pos)/SECTOR_SIZE,_fp->f_seek_pos);
//        GFS_FILE_USED(fp) = FALSE;
//        ++fp;
//    }


		Uint32 start_sector = (_fp->f_seek_pos)/SECTOR_SIZE;
		Uint32 skip_bytes = _fp->f_seek_pos & (SECTOR_SIZE - 1);
		GFS_Seek(_fp->fid, start_sector, GFS_SEEK_SET);
		Sint32 tot_bytes = len + skip_bytes;
		Sint32 tot_sectors = GFS_BYTE_SCT(tot_bytes, SECTOR_SIZE);
		Uint32 readBytes = GFS_Fread(_fp->fid, tot_sectors, ptr, tot_bytes);


		GfsHn       gfs;
		GftrHn      gftr;

		gfs = MNG_FILE(gfs_mng_ptr);
		for (i = 0; i < MNG_OPENMAX(gfs_mng_ptr); i++, gfs++) {
			if (GFS_FILE_USED(gfs) == TRUE) {
			
				if(gfs->flow.finfo.fid!=141)
				{
					GFS_FILE_USED(gfs) = FALSE;
					GFS_Close(gfs);
				}
			}
		}


		_fp->f_seek_pos += (readBytes - skip_bytes);
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
    _impl->batchSeek(off);
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
/*
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
*/