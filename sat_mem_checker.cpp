#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include <sgl.h>
#include <sl_def.h>
#include <sega_sys.h>
#include <sega_mem.h>
#include "sat_mem_checker.h"

#include "saturn_print.h"

void *sat_calloc(size_t nmemb, size_t size) {
	void *mem = NULL;

	mem = (void*)MEM_Calloc(nmemb, size);

	//fprintf_saturn(stdout, "CALLOC: all: %u bytes - 0x%.8X", nmemb * size, mem);

	if (mem == NULL) {
		fprintf_saturn(stdout, "CALLOC: nmemb: %u, size: %u - FAILED", nmemb, size);
		while(1) slSynch();
	}

	return (void*)mem;
}

void *sat_malloc(size_t size) {
	void *mem = NULL;

	mem = (void*)MEM_Malloc(size);

	//fprintf_saturn(stdout, "MALLOC: all: %u bytes - 0x%.8X", size, mem);

	if (mem == NULL) {
		fprintf_saturn(stdout, "MALLOC: size: %u - FAILED", size);
		while(1) slSynch();
	}

	return (void*)mem;
}

void sat_free(void *ptr) {

	if(ptr == NULL) return;

	//fprintf_saturn(stdout, "FREE: ptr: 0x%.8X", ptr);

	MEM_Free(ptr);
	
	return;
}

void *sat_realloc(void *ptr, size_t size) {
	void *mem = NULL;

	if(ptr == NULL) return NULL;

	mem = (void*)MEM_Realloc(ptr, size);

	//fprintf_saturn(stdout, "REALLOC: all: %u bytes - 0x%.8X", size, mem);

	if (mem == NULL) {
		fprintf_saturn(stdout, "REALLOC: ptr: %.8X, size: %u - FAILED", ptr, size);
		while(1) slSynch();
	}

	return (void*)mem;

}

char *sat_strdup(const char *s) {
	char *mem = NULL;

#ifdef _SAT_EXT_DEBUG_
	mem = (char*)sat_malloc(strlen(s) + 1);
#else
	mem = (char*)MEM_Malloc(strlen(s) + 1);
#endif
	memcpy(mem, s, strlen(s));

	mem[strlen(s)] = '\0';

	return (char*)mem;
}

