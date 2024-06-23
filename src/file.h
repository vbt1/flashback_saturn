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

#ifndef __FILE_H__
#define __FILE_H__

#include "intern.h"

struct File_impl;

struct File {
	char name[50];
	File();
	~File();

	File_impl *_impl;

	bool open(const char *filename, const char *directory, const char *mode);
	void openMemoryBuffer(int initialCapacity);
	void close();
	bool ioErr() const;
	uint32_t size();
	void seek(int32_t off);
	uint32_t tell();
	void read(void *ptr, uint32_t len);
	uint8_t readByte();
	uint16_t readUint16LE();
	uint32_t readUint32LE();
	uint16_t readUint16BE();
	uint32_t readUint32BE();
	void write(void *ptr, uint32_t size);
	void writeByte(uint8_t b);
	void writeUint16LE(uint16_t n);
	void writeUint32LE(uint32_t n);
	void writeUint16BE(uint16_t n);
	void writeUint32BE(uint32_t n);
	char* fileName(void);	
};

void dumpFile(const char *filename, const uint8_t *p, int size);

#endif // __FILE_H__
