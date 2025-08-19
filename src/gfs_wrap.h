#ifndef _GFS_BYTE_WRAPPER_
#define _GFS_BYTE_WRAPPER_

//#include "C:/SaturnOrbit/SGL_302j/INC320/sgl.h" // Needed defines
#include <sega_cdc.h> // For cd accessing vars and functions
#include <sega_gfs.h> 
#include <stdio.h>

//#define   OPEN_MAX  (Sint32)20
//#define   DIR_MAX   (Sint32)170

#define   OPEN_MAX  (Sint32)8
#define   DIR_MAX   (Sint32)152


#define SECTOR_SIZE 2048 // Bytes for each sector
#define TOT_SECTOR 6

#define FNAME_SIZE 15

typedef struct {
	GfsHn  fid;       // Pointer to GFS file structure
	long f_size;      // file size in *bytes*
	long  f_seek_pos; // seek pointer position in *bytes*
//	Uint32 file_hash; // file hash 
} GFS_FILE;

void init_GFS();

GFS_FILE *sat_fopen(const char *path, const int position);
int sat_fclose(GFS_FILE *fp);

int sat_fseek(GFS_FILE *stream, long offset, int whence);
//long sat_ftell(GFS_FILE *stream);

size_t sat_fread(void *ptr, size_t size, size_t nmemb, GFS_FILE *stream);
char *sat_fgets(char *s, int size, GFS_FILE *stream);

int sat_feof(GFS_FILE *stream);

int sat_ferror(GFS_FILE *stream);

long sat_fsize(GFS_FILE *stream);

//char *sat_match(const char *path);

#endif

