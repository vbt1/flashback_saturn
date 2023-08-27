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

#include <sgl.h>
#include <sl_def.h>
#include <sega_sys.h>

#include <cstdarg>
#include "util.h"

#include "saturn_print.h"

uint16 g_debugMask;

static char buf[128];

void debug(uint16 cm, const char *msg, ...) {
	//char buf[1024];
	if (/*cm & g_debugMask*/0) {
		va_list va;
		va_start(va, msg);
		vsprintf(buf, msg, va);
		va_end(va);
		fprintf_saturn(stdout, "%s\n", buf);
		//fflush(stdout);
	}
}

void error(const char *msg, ...) {
	//char buf[1024];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf_saturn(stderr, "ERROR: %s!\n", buf);
	
	while(1) slSynch();
	//SYS_Exit(0);
}

void warning(const char *msg, ...) {
	return;
	//char buf[1024];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf_saturn(stderr, "WARNING: %s!\n", buf);
}

void string_lower(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void string_upper(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}
