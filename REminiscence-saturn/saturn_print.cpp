extern "C" {

#include "saturn_print.h"

#include <sgl.h>
#include <sl_def.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static int curline = 1;

#define CARTRAM_ID_NONE     0
#define CARTRAM_ID_8M       0x5a
#define CARTRAM_ID_32M      0x5c
Uint32 CartRAMsize = 0;
Sint16 cartRAMdetected = 0;
Sint16 CartRAM_init(Uint8);


void fprintf_saturn(void *stream, char *format, ...) {
	char outstring[40];
	
	memset(outstring, ' ', 40);
	outstring[39] = '\0';
	slPrint(outstring, slLocate(1,curline%25));
	slPrint(outstring, slLocate(1,(curline%25) + 1));

	va_list args;
    va_start(args, format);
    vsnprintf(outstring, 39, format, args);
    va_end(args);
	outstring[39] = '\0';
	
	slPrint(outstring, slLocate(1,curline%25));
	curline = (curline == 25) ? 1 : curline + 1;	
}

inline void printf_saturn(char *format, ...) {
	char outstring[40];

	memset(outstring, ' ', 40);
	outstring[39] = '\0';
	slPrint(outstring, slLocate(1,curline%25));
	slPrint(outstring, slLocate(1,(curline%25) + 1));

	va_list args;
    va_start(args, format);
    vsnprintf(outstring, 39, format, args);
    va_end(args);
	outstring[39] = '\0';

	slPrint(outstring, slLocate(1,curline%25));
	curline = (curline == 25) ? 1 : curline + 1;	
}

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

void SCU_DMAWait(void) {
	Uint32 res;

	while((res = DMA_ScuResult()) == 2);
	
	if(res == 1) {
		fprintf_saturn(stdout, "SCU DMA COPY FAILED!");
	}
}

}
