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
#include "sat_mem_checker.h"

#include "file.h"
#include "game.h"
#include "systemstub.h"

//#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
//#define LOW_WORK_RAM_SIZE 0x100000
void *__dso_handle = 0;

Uint8 *hwram = NULL;
Uint8 *hwram_ptr = NULL;
Uint8 *hwram_screen = NULL;
Uint8 *soundAddr = NULL;
Uint8 *current_lwram = (Uint8 *)VBT_L_START;
Uint32 position_vram = 0;
Uint32 position_vram_aft_monster = 0;
unsigned int end1 = 0;

void ss_main(void) {
	const int version = kResourceTypeMac;
	int levelNum = 0;
	const Language language = (Language)LANG_EN;
	
	SystemStub *stub = SystemStub_SDL_create();

	Game *g = new Game(stub, ".", ".", levelNum, (ResourceType)version,language);

	g->run();
	delete g;
	delete stub;
	return;
}
