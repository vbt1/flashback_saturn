
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C"
{
#include <sgl.h>
#include <sl_def.h>
#include <sega_sys.h>
}
#include <cstdarg>
#include "util.h"

#include "saturn_print.h"

//uint16 g_debugMask;

//static char buf[128];

void debug(uint16 cm, const char *msg, ...) {
	return;
	//char buf[1024];
//	if (/*cm & g_debugMask*/0) {
//		va_list va;
//		va_start(va, msg);
//		vsprintf(buf, msg, va);
//		va_end(va);
//		fprintf_saturn(stdout, "%s\n", buf);
//slPrint((char *)buf,slLocate(10,12));
		//fflush(stdout);
//	}
}

void error(const char *msg, ...) {
	return;
	//char buf[1024];
//	va_list va;
//	va_start(va, msg);
//	vsprintf(buf, msg, va);
//	va_end(va);
//	fprintf_saturn(stderr, "ERROR: %s!\n", buf);
//slPrint((char *)buf,slLocate(10,12));
//	while(1) slSynch();
//	SYS_Exit(0);
}

void warning(const char *msg, ...) {
	return;
	//char buf[1024];
//	va_list va;
//	va_start(va, msg);
//	vsprintf(buf, msg, va);
//	va_end(va);
//	fprintf_saturn(stderr, "WARNING: %s!\n", buf);
//slPrint((char *)buf,slLocate(10,12));
}

void string_lower(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void string_upper(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}
