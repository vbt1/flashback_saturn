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
#include 	<sl_def.h>
//#include "sega_mem.h"
#include "gfs_wrap.h"
void	*malloc(size_t);
}
#include "saturn_print.h"

#include "file.h"
#include "game.h"
#include "systemstub.h"

//#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
//#define LOW_WORK_RAM_SIZE 0x100000
void *__dso_handle = 0;

static int detectVersion(const char *dataPath) {
	static const struct {
		const char *filename;
		int type;
		const char *name;
	} table[] = {
		{ "FLASHBCK.RSR", kResourceTypeMac, "Macintosh" },
		{ "LEVEL1.MAP", kResourceTypeDOS, "DOS" },
		{ 0, -1, 0 }
	};
	for (int i = 0; table[i].filename; ++i) {
		File f;

		if (f.open(table[i].filename, dataPath, "rb")) {
//			debug(DBG_INFO, "Detected %s version", table[i].name);
//			emu_printf("Detected %s version\n", table[i].name);
			f.close();
			return table[i].type;
		}
	}
	return -1;
}

Uint8 *hwram = NULL;
Uint8 *hwram_ptr = NULL;
Uint8 *hwram_screen = NULL;
Uint8 *current_lwram = (Uint8 *)0x200000;
Uint8 *save_lwram = NULL;
Uint32 position_vram = 0;
Uint32 position_vram_aft_monster = 0;
unsigned int end1 = 600000;//-16384;//538624;

void ss_main(void) {
	const int version = detectVersion("/");
//	g_debugMask = DBG_INFO; // DBG_CUT | DBG_VIDEO | DBG_RES | DBG_MENU | DBG_PGE | DBG_GAME | DBG_UNPACK | DBG_COL | DBG_MOD | DBG_SFX;
	int levelNum = 0;
	const Language language = (Language)LANG_EN;
	
	SystemStub *stub = SystemStub_SDL_create();

	Game *g = new Game(stub, ".", ".", levelNum, (ResourceType)version,language);

	g->run();
	delete g;
	delete stub;
	return;
}
