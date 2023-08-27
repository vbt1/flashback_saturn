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

#include <sega_mem.h>
#include "gfs_wrap.h"
#include "saturn_print.h"

#include "file.h"
#include "game.h"
#include "systemstub.h"

#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
#define LOW_WORK_RAM_SIZE 0x100000

static Version detectVersion(const char *dataPath) {
	static struct {
		const char *filename;
		Version ver;
	} checkTable[] = {
		{ "ENGCINE.BIN", VER_EN },
		{ "FR_CINE.BIN", VER_FR },
		{ "GERCINE.BIN", VER_DE },
		{ "SPACINE.BIN", VER_SP }
	};
	for (uint8 i = 0; i < ARRAYSIZE(checkTable); ++i) {
		File f;
		if (f.open(checkTable[i].filename, dataPath, "rb")) {
			return checkTable[i].ver;
		}
	}
	error("Unable to find data files, check that all required files are present");
	return VER_EN;
}

void ss_main(void) {
	Version ver = detectVersion("/");
	g_debugMask = DBG_INFO; // DBG_CUT | DBG_VIDEO | DBG_RES | DBG_MENU | DBG_PGE | DBG_GAME | DBG_UNPACK | DBG_COL | DBG_MOD | DBG_SFX;

	SystemStub *stub = SystemStub_SDL_create();

	Game *g = new Game(stub, ".", ".", ver);
	
	g->run();
	delete g;
	delete stub;
	return;
}
