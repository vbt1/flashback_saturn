#include <sgl.h>
#include <sega_mem.h>

extern "C" {

void fprintf_saturn(void*, char*, ...);
void printf_saturn(char*, ...);
Sint16 CartRAM_init(Uint8 cs);
void SCU_DMAWait(void);

}


