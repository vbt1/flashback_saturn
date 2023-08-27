extern "C" {
#include	<sgl.h>
#include 	<sl_def.h>
#include 	<sega_mem.h>
#include 	<sega_spr.h>
#include	<sega_csh.h>
#include	<sega_sys.h>
#include	"gfs_wrap.h"
}

#include 	"saturn_print.h"
#include 	"systemstub.h"

#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
#define LOW_WORK_RAM_SIZE 0x80000


#define		SystemWork		0x060ffc00		/* System Variable Address */
#define		SystemSize		(0x06100000-0x060ffc00)		/* System Variable Size */

extern Uint32 _bstart, _bend;

extern void ss_main( void );

extern "C" {
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

	//CartRAM_init(0);
#ifdef _352_CLOCK_
	slInitSystem(TV_352x224, (TEXTURE*)NULL, 1); // Init SGL
#else
	slInitSystem(TV_320x224, (TEXTURE*)NULL, 1); // Init SGL
#endif

	CSH_Init(CSH_4WAY);
	MEM_Init(LOW_WORK_RAM, LOW_WORK_RAM_SIZE); // Use low work ram for the sega mem library
	//MEM_Init(0x22600000, 0x200000);

#ifdef _PAR_UPLOAD_
	cdUnlock(); // Unlock the cd drive
#endif
	DMA_ScuInit(); // Init for SCU DMA
	init_GFS(); // Initialize GFS system
	SPR_InitSlaveSH();
	/* Application Call */
	ss_main();

	return 0;
}

