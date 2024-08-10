//#define SLAVE_SOUND 1

extern "C" {
//#include	<sgl.h>
#include 	<sl_def.h>
#include 	<sega_mem.h>
//#include 	<sega_spr.h>
//#include	<sega_csh.h>
#include	<sega_sys.h>
#include	"gfs_wrap.h"
#include <stdarg.h>
#include <string.h>
void *memset4_fast(void *, long, size_t);
}

#include 	"saturn_print.h"
#include 	"systemstub.h"
#include "sat_mem_checker.h"

#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))

#define		SystemWork		0x060ffc00		/* System Variable Address */
#define		SystemSize		(0x06100000-0x060ffc00)		/* System Variable Size */
extern Uint32 _bstart, _bend;

extern void ss_main( void );

extern "C" {
	TEXTURE tex_spr[4];
//	extern void DMA_ScuInit(void);
}

int	main( void )
{
	Uint8	*dst;
	Uint32	i;

	/* 1.Zero Set .bss Section */
	for( dst = (Uint8 *)&_bstart; dst < (Uint8 *)&_bend; dst++ ) {
		*dst = 0;
	}
	/* 2.ROM has data at end of text; copy it. */

	/* 3.SGL System Variable Clear */
	for( dst = (Uint8 *)SystemWork, i = 0;i < SystemSize; i++) {
		*dst = 0;
	}

	init_GFS(); // Initialize GFS system

	int id = CartRAM_init(0);
    if(id!=0x5c)
    {
		slInitSystem(TV_320x224, (TEXTURE*)tex_spr, 1); // Init SGL		
		slTVOn();
		slPrint("No ram cart found",slLocate(10,12));
		slPrint("Please insert 4Mb",slLocate(10,14));
//		emu_printf("id %x\n",id);
		slSynch();
		while(1);
    }
#ifdef _352_CLOCK_
	slInitSystem(TV_320x224, (TEXTURE*)tex_spr, 1); // Init SGL
#else
	slInitSystem(TV_640x448, (TEXTURE*)tex_spr, 1); // Init SGL
//	slSetSprTVMode(TV_320x224); // Init SGL

//	memset4_fast((void *)LOW_WORK_RAM_START,0x00,LOW_WORK_RAM_SIZE);
//	CSH_Init(CSH_4WAY);
	MEM_Init(LOW_WORK_RAM_START, LOW_WORK_RAM_SIZE); // Use low work ram for the sega mem library
//	slTVOff();
//	
	slBitMapNbg0(COL_TYPE_256, BM_512x512, (void *)VDP2_VRAM_B0);
	slBitMapNbg1(COL_TYPE_256, BM_512x512, (void*)VDP2_VRAM_A0); 
//	slZoomNbg1(toFIXED(0.8), toFIXED(1.0));
	slZdspLevel(7); // vbt : ne pas d?placer !!!
	
//	slTVOn();
	slSynch();
#endif

#ifdef _PAR_UPLOAD_
	cdUnlock(); // Unlock the cd drive
#endif
//	DMA_ScuInit(); // Init for SCU DMA
#ifdef SLAVE_SOUND

	SPR_InitSlaveSH();
#endif
	/* Application Call */
	ss_main();

	return 0;
}

#define CS1(x)                  (0x24000000UL + (x))
extern void emu_printf(const char *format, ...);

void emu_printf(const char *format, ...)
{
   static char emu_printf_buffer[128];
   char *s = emu_printf_buffer;
   volatile uint8_t *addr = (volatile uint8_t *)CS1(0x1000);
   va_list args;

   va_start(args, format);
   (void)vsnprintf(emu_printf_buffer, 256, format, args);
   va_end(args);

   while (*s)
      *addr = (uint8_t)*s++;
}

#define CARTRAM_ID_NONE     0
#define CARTRAM_ID_8M       0x5a
#define CARTRAM_ID_32M      0x5c
Uint32 CartRAMsize = 0;
Sint16 cartRAMdetected = 0;

Sint16 CartRAM_init(Uint8 cs) {
	Uint32 setReg, refReg, *DRAM0, *DRAM1;
	Uint8 id;
	
	id = *((Uint8 *)0x24ffffff);
    if(id == 0x5a) {
		CartRAMsize = 0x80000; // 512kb (*2 banks)
    } else if(id == 0x5c) {
        CartRAMsize = 0x200000; // 2Mb (*2 banks)
    } else {
        CartRAMsize = 0x0; // No Connection
        return -1;
    }

	*((Uint16 *)0x257efffe) = 1;
	setReg = refReg = 0;

	if(cs == 0) {
        // set cs0 for 32MBit
        setReg |= 1 << 29;  // After-READ precharge insert bit
        setReg |= 3 << 24;  // Burst cycle wait number
        setReg |= 3 << 20;  // Normal cycle wait number
    } else {
        // set cs1 for 8MBit
        setReg |= 1 << 28;  // external wait effective bit
        setReg |= 15 << 24; // Burst cycle wait number
        setReg |= 15 << 20; // Normal cycle wait number
    }
	
	*((Uint32 *)0x25fe00B0) = setReg;

    DRAM0 = (Uint32 *)0x22400000;
    DRAM1 = (Uint32 *)0x22600000;

	memset(DRAM0, 0, CartRAMsize); // Clean up the expanded ram.
	memset(DRAM1, 0, CartRAMsize);

	return id;
}



