#ifndef _SAT_MEM_CHECK_
#define _SAT_MEM_CHECK_

//#include <sega_mem.h>
#include <stdlib.h>

void *sat_calloc(size_t nmemb, size_t size);
void *sat_malloc(size_t size);
void sat_free(void *ptr);
void *std_calloc(size_t nmemb, size_t size);
void *std_malloc(size_t size);

void *sat_realloc(void *ptr, size_t size);
char *sat_strdup(const char *s);

void memcpyl(void *, void *, int);
void *memset4_fast(void *, long, size_t);
#define SAT_ALIGN(a) ((a+3)&~3)


#define LOW_WORK_RAM_START 0x00290000 // Beginning of LOW WORK RAM (1Mb)
#define LOW_WORK_RAM_SIZE 0x300000-LOW_WORK_RAM_START

#define VBT_L_START    ((volatile void *)(0x200000))
//#define LOW_WORK_RAM_START    ((volatile void *)(0x290000))
#define ADR_WORKRAM_L_END      ((volatile void *)0x300000)


#endif
