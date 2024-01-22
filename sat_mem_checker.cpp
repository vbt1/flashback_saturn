#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "stdlib.h"
//#include <cstdlib>
extern "C" {
#include "stdlib.h"
#include <sgl.h>
#include <sl_def.h>
#include <sega_sys.h>
#include <sega_mem.h>
#include "sat_mem_checker.h"
extern Uint8 *objvbt;
typedef double MemAlign;                        /* 64ビットのアライメント    */
union mem_head {                                /* セルのヘッダ              */
    struct {
        union mem_head *next;                   /* 次のセルポインタ          */
        unsigned size;                          /* セルのサイズ              */
    }s;
    MemAlign damy;
};

typedef union mem_head MemHead;                 /* セルヘッダデータ型        */
void	*malloc(size_t);
void	free (void *) ;
extern MemHead *MEM_empty_top;
}
#include "saturn_print.h"


extern "C" void __cxa_pure_virtual() { while (1); }

void *sat_calloc(size_t nmemb, size_t size) {
	void *mem = NULL;

	mem = (void*)MEM_Calloc(nmemb, size);

	if (mem == NULL) {
		emu_printf("CALLOC: nmemb: %u, size: %u - FAILED\n", nmemb, size);
	}
//	emu_printf("CALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;
}

void *sat_malloc(size_t size) {
	void *mem = NULL;
	size = (size + 7) & ~7; // pour alignement
	
	mem = (void*)MEM_Malloc(size);

	if (mem == NULL) {
		emu_printf("MEM_MALLOC: size: %u - FAILED\n", size);

		mem = (uint8_t *)0x25C04000;
//		mem = (void*)malloc(size);;
//		if (!dst) {
			
//			emu_printf("Failed to allocate %d bytes for LZSS in HWRAM\n", decodedSize);		
//			dst = (uint8_t *)0x25C04000;

//		}
	}
	return (void*)mem;
}

void sat_free(void *ptr) {
#define ADR_WORKRAM_L_START    ((volatile void *)0x202000)
#define ADR_WORKRAM_L_END      ((volatile void *)0x300000)

	if(ptr == NULL || ptr == objvbt)
		return;
	if(((int)ptr) == 0x25C60000 || ((int)ptr) == 0x25C04000 || ((int)ptr) == (0x25C80000-60000)) 
	{
//		slPrint("((int)ptr) == 0x25C60000 || ((int)ptr) == 0x25C04000",slLocate(3,21));
//		char toto[50];
//		sprintf(toto,"address = %p",ptr);
//		slPrint(toto,slLocate(3,21));
//		ptr = NULL;
		return;		
	}


//	emu_printf("FREE: addr: %p %p\n", ptr,MEM_empty_top);		

	if((ptr >= ADR_WORKRAM_L_START) && (ptr < ADR_WORKRAM_L_END))
	{
		MEM_Free(ptr);
	}
	else
	{
//		emu_printf("FREE: addr: %p\n", ptr);
		free(ptr);
	}
	ptr = NULL;
	return;
}

void *sat_realloc(void *ptr, size_t size) {
	void *mem = NULL;

	if(ptr == NULL) return NULL;
	size = (size + 7) & ~7; // pour alignement
	mem = (void*)MEM_Realloc(ptr, size);

	if (mem == NULL) {
		emu_printf("REALLOC: ptr: %.8X, size: %u - FAILED\n", ptr, size);
	}
//	emu_printf("REALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;

}

void *std_malloc(size_t size) {
	void *mem = NULL;
	size = (size + 7) & ~7; // pour alignement	
	mem = (void*)malloc(size);

	if (mem == NULL) {
		emu_printf("STD_MALLOC: size: %u - FAILED\n", size);
	}
//	else
//emu_printf("MALLOC success\n");			

//	emu_printf("STD_MALLOC: addr: %p, size: %u \n", mem, size);

	return (void*)mem;
}

void *std_calloc(size_t nmemb, size_t size) {
	void *mem = NULL;

	mem = (void*)calloc(nmemb, size);

	if (mem == NULL) {
		emu_printf("STD_CALLOC: nmemb: %u, size: %u - FAILED\n", nmemb, size);
	}
//	emu_printf("STD_CALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;
}

