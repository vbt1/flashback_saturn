#ifndef _SAT_MEM_CHECK_
#define _SAT_MEM_CHECK_

#include <sega_mem.h>
#include <stdlib.h>

void *sat_calloc(size_t nmemb, size_t size);
void *sat_malloc(size_t size);
void sat_free(void *ptr);
void *sat_realloc(void *ptr, size_t size);
char *sat_strdup(const char *s);
#endif
