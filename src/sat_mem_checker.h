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
#define SAT_ALIGN8(a) ((a+15)&~15)


#define LOW_WORK_RAM_START 0x00280000 // Beginning of LOW WORK RAM (1Mb)
#define LOW_WORK_RAM_SIZE 0x300000-LOW_WORK_RAM_START

#define VBT_L_START    ((volatile void *)(0x200000))
//#define LOW_WORK_RAM_START    ((volatile void *)(0x290000))
#define ADR_WORKRAM_L_END      ((volatile void *)0x300000)
#define HWRAM_SCREEN_SIZE 50000//(128*240*2)+(64*240)
// si on modifie scratch et front, la sequence de fin a des glitchs ...
#define SCRATCH 0x2e7000
#define FRONT   0x2aeff8
#define CUTCMP1 0x2AAFF8				// CAILLOU
#define CUTCMP2 CUTCMP1+SAT_ALIGN(6361) // ASC
#define CUTCMP3 CUTCMP2+SAT_ALIGN(2472)
#define CUTCMP4 CUTCMP3+SAT_ALIGN(2296) // SERRUE
#define CUTCMP5 CUTCMP4+SAT_ALIGN(3512) // MEMO
#define VRAM_MAX 0x67000 // ne pas toucher


#define SPR_ELEVATOR 273 //1560
#define SPR_METRO    616 //1903
#endif
