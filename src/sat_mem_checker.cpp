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
extern Uint8 *hwram;
//extern Uint8 *save_lwram;
extern Uint8 *current_lwram;
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


//extern "C" void __cxa_pure_virtual() { while (1); }

void sat_free(void *ptr) {

	if(ptr == NULL || ptr == hwram)
		return;
	
//	if(((int)ptr) == 0x25C60000 || ((int)ptr) == 0x25C04000 || ((int)ptr) == (0x25C80000-60000)) 
	if((int)ptr == 0x25C04000) 
	{
// 	vbt : 0x25C04000 toujour utilisé	
//		slPrint("((int)ptr) == 0x25C60000 || ((int)ptr) == 0x25C04000",slLocate(3,21));
//		char toto[50];
//		sprintf(toto,"address = %p",ptr);
//		slPrint(toto,slLocate(3,21));
//		ptr = NULL;
		return;		
	}
		
	if((ptr >= VBT_L_START) && (ptr < (volatile void *)LOW_WORK_RAM_START))
	{
//		emu_printf("NO FREE: addr: %p\n", ptr);
		ptr = NULL;
	}	
	else
	{
		emu_printf("FREE: addr: %p\n", ptr);
		ptr = NULL;
	//	free(ptr);
	}

	return;
}
