#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "stdlib.h"
//#include <cstdlib>
extern "C" {
#include "stdlib.h"
#include <sgl.h>
#include <sl_def.h>
#include <sega_mem.h>
#include "sat_mem_checker.h"
extern Uint8 *hwram;
 Uint8 *save_lwram;
}
#include "saturn_print.h"

void *sat_malloc(size_t size) {
	void *mem = NULL;

	mem = (void*)MEM_Malloc(SAT_ALIGN(size));

	if (mem == NULL) {
//		do
		{
		emu_printf("MEM_MALLOC: size: %u - FAILED\n", size);
		}
//while(1);
//		mem = (uint8_t *)0x25C04000;
//		mem = (void*)malloc(size);;
//		if (!dst) {
			emu_printf("MEM_MALLOC Failed to allocate %d bytes in LHWRAM\n", size);
//			dst = (uint8_t *)0x25C04000;

//		}
	}
	emu_printf("MEM_MALLOC: size: %u - %p\n", size, mem);
	save_lwram = (uint8_t*)((int)mem & 0x002FFFFF);

	return (void*)save_lwram;
}

void sat_free(void *ptr) {

	if(ptr == NULL || ptr == hwram)
		return;

	emu_printf("FREE to NULL: addr: %p\n", ptr);
	ptr = NULL;

	return;
}
