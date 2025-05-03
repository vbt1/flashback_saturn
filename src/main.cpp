
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
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
