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
//	mem = (void*)calloc(nmemb, size);

	//fprintf_saturn(stdout, "CALLOC: all: %u bytes - 0x%.8X", nmemb * size, mem);

	if (mem == NULL) {
		emu_printf("CALLOC: nmemb: %u, size: %u - FAILED\n", nmemb, size);
//		while(1) slSynch();
	}
//	int *val = (int *)mem;	
	emu_printf("CALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;
}

void *sat_malloc(size_t size) {
	void *mem = NULL;
	mem = (void*)MEM_Malloc(size);
//	mem = (void*)malloc(size);

	//fprintf_saturn(stdout, "MALLOC: all: %u bytes - 0x%.8X\n", size, mem);

	if (mem == NULL) {
		emu_printf("MEM_MALLOC: size: %u - FAILED\n", size);
		


		mem = (uint8_t *)0x25C04000;
//		mem = (void*)malloc(size);;
//		if (!dst) {
			
//			emu_printf("Failed to allocate %d bytes for LZSS in HWRAM\n", decodedSize);		
//			dst = (uint8_t *)0x25C04000;

//		}
	
		
/*		
		mem = (void*)std_malloc(size);
		
		if (mem == NULL) {
			emu_printf("STD_MALLOC: size: %u - FAILED\n", size);		
		}*/
	}
//	else
//emu_printf("MALLOC success\n");			

//	int *val = (int *)mem;	
	emu_printf("MALLOC: addr: %p, size: %u \n", mem, size);
	
//slPrintHex((int)val,slLocate(10,15));

	return (void*)mem;
}

void sat_free(void *ptr) {
	emu_printf("FREE: addr: %p %p\n", ptr,MEM_empty_top);
	
	if(ptr == NULL) return;

		
#define ADR_WORKRAM_L_START    ((volatile void *)0x200000)
#define ADR_WORKRAM_L_END      ((volatile void *)0x300000)		
	if((ptr >= ADR_WORKRAM_L_START) && (ptr < ADR_WORKRAM_L_END))
	{
		//fprintf_saturn(stdout, "FREE: ptr: 0x%.8X", ptr);
		MEM_Free(ptr);
	}
	else
		free(ptr);
	
	ptr = NULL;
//	free(ptr);
//	emu_printf("AFTER FREE: addr: %p %p\n", ptr,MEM_empty_top);	
	return;
}

void *sat_realloc(void *ptr, size_t size) {
	void *mem = NULL;

	if(ptr == NULL) return NULL;

	mem = (void*)MEM_Realloc(ptr, size);
//	mem = (void*)realloc(ptr, size);

	//fprintf_saturn(stdout, "REALLOC: all: %u bytes - 0x%.8X", size, mem);

	if (mem == NULL) {
		emu_printf("REALLOC: ptr: %.8X, size: %u - FAILED\n", ptr, size);
	}
//	int *val = (int *)mem;	
		emu_printf("REALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;

}

void *std_malloc(size_t size) {
	void *mem = NULL;
	mem = (void*)malloc(size);

	//fprintf_saturn(stdout, "MALLOC: all: %u bytes - 0x%.8X\n", size, mem);

	if (mem == NULL) {
		emu_printf("STD_MALLOC: size: %u - FAILED\n", size);
	}
//	else
//emu_printf("MALLOC success\n");			

//	int *val = (int *)mem;	
	emu_printf("STD_MALLOC: addr: %p, size: %u \n", mem, size);
	
//slPrintHex((int)val,slLocate(10,15));

	return (void*)mem;
}

void std_free(void *ptr) {
	emu_printf("STD_FREE: addr: %p\n", ptr);
	if(ptr == NULL) return;

	free(ptr);
	
	ptr = NULL;
	
	return;
}

void *std_calloc(size_t nmemb, size_t size) {
	void *mem = NULL;

	mem = (void*)calloc(nmemb, size);

	//fprintf_saturn(stdout, "CALLOC: all: %u bytes - 0x%.8X", nmemb * size, mem);

	if (mem == NULL) {
		emu_printf("STD_CALLOC: nmemb: %u, size: %u - FAILED\n", nmemb, size);
//		while(1) slSynch();
	}
//	int *val = (int *)mem;	
//	emu_printf("STD_CALLOC: addr: %p, size: %u\n", mem, size);
	return (void*)mem;
}

/*
char *sat_strdup(const char *s) {
	char *mem = NULL;

#ifdef _SAT_EXT_DEBUG_
	mem = (char*)sat_malloc(strlen(s) + 1);
#else
//	mem = (char*)MEM_Malloc(strlen(s) + 1);
//	mem = (char*)malloc(strlen(s) + 1);
#endif
	memcpy(mem, s, strlen(s));

	mem[strlen(s)] = '\0';

	return (char*)mem;
}
*/

