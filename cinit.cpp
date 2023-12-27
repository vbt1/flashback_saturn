extern "C" {
//#include	<sgl.h>
#include 	<sl_def.h>
#include 	<sega_mem.h>
#include 	<sega_spr.h>
#include	<sega_csh.h>
#include	<sega_sys.h>
#include	"gfs_wrap.h"
#include <stdarg.h>
#include <string.h>
}

#include 	"saturn_print.h"
#include 	"systemstub.h"

#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
#define LOW_WORK_RAM_SIZE 0x100000
#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))

#define		SystemWork		0x060ffc00		/* System Variable Address */
#define		SystemSize		(0x06100000-0x060ffc00)		/* System Variable Size */
//extern Uint8 newZoom;
extern Uint32 _bstart, _bend;

extern void ss_main( void );

extern "C" {
TEXTURE tex_spr[4];
	extern void DMA_ScuInit(void);
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
	//CartRAM_init(0);
#ifdef _352_CLOCK_
	slInitSystem(TV_320x224, (TEXTURE*)tex_spr, 1); // Init SGL
#else
	slInitSystem(TV_640x448, (TEXTURE*)tex_spr, 1); // Init SGL
 //   slSetScrTVMode(TV_640x448);
 //   slInitSystem(TV_320x240, (TEXTURE*)NULL, 1);
 //   slSetScrTVMode(TV_320x240);	
    slPriorityNbg0(4);
    slPriorityNbg1(5);	
//	slCharNbg0(COL_TYPE_256, CHAR_SIZE_2x2);
//	slCharNbg1(COL_TYPE_256, CHAR_SIZE_2x2);
//	slPageNbg0((void *)NBG0_CEL_ADR, 0, PNB_1WORD|CN_10BIT);
//	slPageNbg1((void *)NBG1_CEL_ADR, 0, PNB_2WORD);
//	slPlaneNbg0(PL_SIZE_2x1);
//	slPlaneNbg1(PL_SIZE_2x1);	
	
	/*
extern Uint16 VDP2_RAMCTL;	
	VDP2_RAMCTL = VDP2_RAMCTL & 0xFCFF;
	extern Uint16 VDP2_TVMD;
	VDP2_TVMD &= 0xFEFF;
*/

//    slScrAutoDisp(NBG0ON);	
//	slSynch();
	memset((void *)LOW_WORK_RAM,0x00,LOW_WORK_RAM_SIZE);
//	CSH_Init(CSH_4WAY);
	MEM_Init(LOW_WORK_RAM, LOW_WORK_RAM_SIZE); // Use low work ram for the sega mem library
	
	slTVOff();
	slScrAutoDisp(NBG1ON|SPRON);
    slBitMapNbg0(COL_TYPE_256, BM_512x512, (void *)VDP2_VRAM_B0);
//	slZoomNbg0(toFIXED(0.727272), toFIXED(1.0));
	slZoomNbg1(toFIXED(0.8), toFIXED(1.0));
	slPrioritySpr0(6);

	slZdspLevel(7); // vbt : ne pas d?placer !!!
//    slBitMapNbg1(COL_TYPE_256, BM_512x256, (void *)VDP2_VRAM_B0);
	
slTVOn();
	slSynch();
//	newZoom = 2;	

	
#endif


//	
	//MEM_Init(0x22600000, 0x200000);

#ifdef _PAR_UPLOAD_
	cdUnlock(); // Unlock the cd drive
#endif

//	DMA_ScuInit(); // Init for SCU DMA
//	init_GFS(); // Initialize GFS system

//	SPR_InitSlaveSH();

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



