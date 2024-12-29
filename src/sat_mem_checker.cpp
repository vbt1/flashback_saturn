extern "C" {
#include <sgl.h>
#include "sat_mem_checker.h"
extern Uint8 *hwram;
}
#include "saturn_print.h"

void sat_free(void *ptr) {

	if(ptr == NULL || ptr == hwram)
		return;
		
	emu_printf("FREE to NULL: addr: %p\n", ptr);
	ptr = NULL;

	return;
}
