extern "C" {
#include 	<sl_def.h>
#include	<sega_sys.h>
#include	"gfs_wrap.h"
#include <stdarg.h>
#include <string.h>
void *memset4_fast(void *, long, size_t);
}

#include 	"systemstub.h"
#include "sat_mem_checker.h"

#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))

#define		SystemWork		0x060ffc00		/* System Variable Address */
#define		SystemSize		(0x06100000-0x060ffc00)		/* System Variable Size */
extern Uint32 _bstart, _bend;

extern void ss_main( void );

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

	slInitSystem(TV_640x448, (TEXTURE*)NULL, 1); // Init SGL
//	memset4_fast((void *)LOW_WORK_RAM_START,0x00,LOW_WORK_RAM_SIZE);

	slBitMapNbg0(COL_TYPE_256, BM_512x512, (void*)VDP2_VRAM_B0);
	slBitMapNbg1(COL_TYPE_256, BM_512x512, (void*)VDP2_VRAM_A0); 
//	slZoomNbg1(toFIXED(0.8), toFIXED(1.0));
	slZdspLevel(7); // vbt : ne pas d?placer !!!
	slSynch();

//	DMA_ScuInit(); // Init for SCU DMA
	ss_main();
	return 0;
}

#define CS1(x)                  (0x24000000UL + (x))
extern void emu_printf(const char *format, ...);

#if 1
void emu_printf(const char *format, ...)
{
#if 1
   static char emu_printf_buffer[128];
   char *s = emu_printf_buffer;
   volatile uint8_t *addr = (volatile uint8_t *)CS1(0x1000);
   va_list args;

   va_start(args, format);
   (void)vsnprintf(emu_printf_buffer, 256, format, args);
   va_end(args);

   while (*s)
      *addr = (uint8_t)*s++;
#endif  
}
#endif