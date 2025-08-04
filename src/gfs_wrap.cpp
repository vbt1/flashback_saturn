//#pragma GCC optimize ("O2")

extern "C" {

#include <string.h>
#include <strings.h>

#include <ctype.h>

//#include <sgl.h>
//#include <sl_def.h>
//#include <sega_cdc.h>
//#include <sega_gfs.h>
//#include <sega_mem.h>
#include "gfs_wrap.h"
#include "sat_mem_checker.h"
extern Uint8 *current_lwram;
char 	*strtok (char *__restrict, const char *__restrict);
int	 strncasecmp(const char *, const char *, size_t) __pure;
GfsDirTbl gfsDirTbl;
GfsDirName gfsDirName[DIR_MAX];
Uint32 gfsLibWork[GFS_WORK_SIZE(OPEN_MAX)/sizeof(Uint32)];     
Sint32 gfsDirN;
extern Sint32  gfcd_fatal_err; /* OPEN, NODISC, FATALの保持                */
extern GfsMng   *gfs_mng_ptr;
}

#include "saturn_print.h"

//#define CACHE_SIZE (SECTOR_SIZE * 20)
#define CACHE_SIZE (SECTOR_SIZE * 8)

static char satpath[25];
//static char current_path[15][16];
//static char current_path[3][16];

static Uint32 current_cached = 0;
static Uint8 cache[CACHE_SIZE] __attribute__ ((aligned (4)));

//static Uint8  fully_cached = 0; // If file is cached from start to finish
static Uint32 cache_offset = 0;

// Used for initialization and such


Uint8 dir_depth = 0;

Uint32 hashString(const char *string) {
        char c, *f ;
        Uint32 n ;

        if (!string)
                return 0 ;

        n = 0 ;
        f = (char *)string ;

        while ((c = *f) != 0) {
                n = (n << 7) + (n << 1) + n + c ;
                f++ ;
        }

        return n ;
}
/*
void back_to_root() {
	while(dir_depth) {
		GFS_LoadDir(1, &gfsDirTbl); // to the parent
		GFS_SetDir(&gfsDirTbl);

		memset(current_path[dir_depth], 0, 16);
		dir_depth--;
	}
}

Sint32 crawl_dir(char *token) {
	Sint32 ret;
	Sint32 id;

	if(strcmp("..", token) == 0) { // parent
		GFS_LoadDir(1, &gfsDirTbl);
		ret = GFS_SetDir(&gfsDirTbl);
		if(dir_depth > 0) { 
			memset(current_path[dir_depth], 0, 16);
			dir_depth--;
		}
	} else if (strcmp(".", token) == 0) {
		ret = 0; // just do nothing in this case
	} else { // normal entry
		id = GFS_NameToId((Sint8*)token);
		if(id < 0) return -1;

		GFS_LoadDir(id, &gfsDirTbl);
		ret = GFS_SetDir(&gfsDirTbl);
		if (ret < 0) return ret;

		dir_depth++;
		strcpy(current_path[dir_depth], token);
	}

	return ret;
}


void errGfsFunc(void *obj, int ec)
{
	char texte[50];
	sprintf(texte, "ErrGfs %X %X",obj, ec); 
	texte[49]='\0';
	do{
//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"planté gfs",70,130);
//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)texte,70,140);
	emu_printf("%s\n", texte);

    int      ret;
    GfsErrStat  stat;
    GFS_GetErrStat(&stat);
    ret = GFS_ERR_CODE(&stat);

	sprintf(texte, "ErrGfsCode %X",ret); 
	texte[49]='\0';

//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)texte,70,150);
	emu_printf("%s\n", texte);
//	wait_vblank();

	}while(1);
}
*/

#define GFS_LOCAL
#define GFCD_ERR_OK             0       /* 正常終了 */
#define GFCD_ERR_WAIT           -1      /* 処理待ち */
#define GFCD_ERR_NOCDBLK        -2      /* CDブロックが接続されていない */
#define GFCD_ERR_NOFILT         -3      /* 空き絞りがない */
#define GFCD_ERR_NOBUF          -4      /* 空き区画がない */
#define GFCD_ERR_INUSE          -5      /* 指定された資源が使用中 */
#define GFCD_ERR_RANGE          -6      /* 引数が範囲外 */
#define GFCD_ERR_UNUSE          -7      /* 未確保のものを操作しようとした */
#define GFCD_ERR_QFULL          -8      /* コマンドキューがいっぱい */
#define GFCD_ERR_NOTOWNER       -9      /* 非所有者が資源を操作しようとした */
#define GFCD_ERR_CDC            -10     /* CDCからのエラー */
#define GFCD_ERR_CDBFS          -11     /* CDブロックファイルシステムエラー */
#define GFCD_ERR_TMOUT          -12     /* タイムアウト */
#define GFCD_ERR_OPEN           -13     /* トレイが開いている */
#define GFCD_ERR_NODISC         -14     /* ディスクが入っていない */
#define GFCD_ERR_CDROM          -15     /* CD-ROMでないディスクが入っている */
#define GFCD_ERR_FATAL          -16     /* ステータスがFATAL */
/*
#define MNG_ERROR(mng)          ((mng)->error)

GFS_LOCAL Sint32 gfs_mngSetErrCode(Sint32 code)
{
    GfsErrStat  *err;

    switch (gfcd_fatal_err) {
    case    GFCD_ERR_OK:
        break;
#if !defined(DEBUG_LIB)
    case    GFCD_ERR_OPEN:
        code = GFS_ERR_CDOPEN;
        break;
    case    GFCD_ERR_NODISC:
        code = GFS_ERR_CDNODISC;
        break;
#endif
    case    GFCD_ERR_FATAL:
        code = GFS_ERR_FATAL;
        break;
    default:
        break;
    }
    err = &MNG_ERROR(gfs_mng_ptr);
    GFS_ERR_CODE(err) = code;

    if ((code != GFS_ERR_OK)&&(GFS_ERR_FUNC(err) != NULL)) {
        GFS_ERR_FUNC(err)(GFS_ERR_OBJ(err), code);
    }
    return code;
}

void GFS_SetErrFunc(GfsErrFunc func, void *obj)
{
    GFS_ERR_FUNC(&MNG_ERROR(gfs_mng_ptr)) = func;
    GFS_ERR_OBJ(&MNG_ERROR(gfs_mng_ptr)) = obj;
    gfs_mngSetErrCode(GFS_ERR_OK);
}
*/

void init_GFS() { //Initialize GFS system

	CDC_CdInit(0x00,0x00,0x05,0x0f);

    GFS_DIRTBL_TYPE(&gfsDirTbl) = GFS_DIR_NAME;
    GFS_DIRTBL_DIRNAME(&gfsDirTbl) = gfsDirName;
    GFS_DIRTBL_NDIR(&gfsDirTbl) = DIR_MAX;
    gfsDirN = GFS_Init(OPEN_MAX, gfsLibWork, &gfsDirTbl);
	dir_depth = 0;
//	GFS_SetErrFunc((GfsErrFunc)errGfsFunc, NULL );
	memset4_fast(cache, 0, CACHE_SIZE);
}

GFS_FILE *sat_fopen(const char *path) {
//slSynch();
	memset(satpath, 0, 25);

	if (path == NULL) 
	{	
		return NULL; // nothing to do...
	}
	Uint16 idx;
	static GFS_FILE fp[1];
	idx = 0;
	if (path[idx] == '\0')
	{
		return NULL; // Malformed path
	}
	Uint16 path_len = strlen(path);
	Uint16 tokens = 0; // How many "entries" we have in this path?
	strncpy(satpath, path, path_len + 1); // Prepare the string to work on

	if ((char*)strtok(satpath, "/") != NULL) tokens++; // it's not an empty path...
	else return NULL; // empty path... nonsense.
	while((char*)strtok(NULL, "/") != NULL) tokens++; // count remaining

	strncpy(satpath, path, path_len + 1);

	char *path_token = (char*)strtok(satpath, "/");
/*
	Uint8 sameDir = 1;
	if((tokens - 1) == dir_depth)
		for (idx = 0; idx < (tokens - 1) && idx < 14; idx++) {
			if (strncasecmp(path_token, current_path[idx + 1], 15) != 0) {
				sameDir = 0;
				break;
			}
			path_token = (char*)strtok(NULL, "/"); // next entry
		}
	else
		sameDir = 0;

	if(!sameDir) {
		back_to_root();
	}
*/
	strncpy(satpath, path, path_len + 3);
	for (idx = 0; idx < strlen(satpath); idx++)
		satpath[idx] = toupper(satpath[idx]);

	if(!strstr(satpath, "."))
		strcat(satpath, ".");

	/* We now have the number of entries: N-1 are dirs, the last is the file */

	// Now, crawl through the dir path
	Sint32 ret = 0;
	path_token = (char*)strtok(satpath, "/");
	for (idx = 0; idx < (tokens - 1); idx++) {
/*		if(!sameDir)
			ret = crawl_dir(path_token);
*/
		if (ret < 0) break; // FIXME: (not sure checking for < 1 is ok) Argh, something is wrong in the path! 

		path_token = (char*)strtok(NULL, "/"); // next entry
	}

	GfsHn fid = NULL;
	// OPEN FILE
	if(ret >= 0) // Open only if we are sure we traversed the path correctly
	{
		fid = GFS_Open(GFS_NameToId((Sint8*)path_token));
	}
	
	if(fid != NULL) { // Opened!
		Sint32 fsize;
		GFS_SetTmode(fid, GFS_TMODE_SCU); // DMA transfer by SCU

		// Encapsulate the file data
		fp->fid = fid;
		fp->f_seek_pos = 0;
		GFS_GetFileInfo(fid, NULL, NULL, &fsize, NULL);
		fp->f_size = fsize;
		fp->file_hash = hashString(path);

		if((current_cached != fp->file_hash) && (fp->f_size < CACHE_SIZE)) {
			current_cached = fp->file_hash;
//					emu_printf("GFS_Seek1 %d\n",fsize);
			Sint32 tot_sectors;
			GFS_Seek(fp->fid, 0, GFS_SEEK_SET);
			tot_sectors = GFS_ByteToSct(fp->fid, fp->f_size);
			memset((Uint8*)cache, 0, fp->f_size);

//			fully_cached = 1;
			cache_offset = 0;
//					emu_printf("GFS_Fread1 in cache\n");
			GFS_Fread(fp->fid, tot_sectors, (Uint8*)cache, fp->f_size);
		} else if ((current_cached != fp->file_hash) && (fp->f_size >= CACHE_SIZE)) {
			current_cached = fp->file_hash;
//					emu_printf("GFS_Seek2 read partly file\n");
			Sint32 tot_sectors;
			GFS_Seek(fp->fid, 0, GFS_SEEK_SET);
			tot_sectors = GFS_ByteToSct(fp->fid, CACHE_SIZE);
			memset4_fast((Uint8*)cache, 0, CACHE_SIZE);

//			fully_cached = 0;
			cache_offset = 0;
//					emu_printf("GFS_Fread2 fid %d size %d cache %d\n", fid, fsize, CACHE_SIZE);	
			GFS_Fread(fp->fid, tot_sectors, (Uint8*)cache, CACHE_SIZE);
		}
	}
	// Now... get back to the roots!
	//back_to_root();
	return fp;
}

int sat_fclose(GFS_FILE* fp) {
//emu_printf("sat_fclose\n");	
	GFS_Close(fp->fid);
//	sat_free(fp);

	return 0; // always ok :-)
}

int sat_fseek(GFS_FILE *stream, long offset, int whence) {
	if(stream == NULL) return -1;

	switch(whence) {
		case SEEK_SET:
			if(offset < 0 || offset >= stream->f_size)
			{
//				emu_printf("SEEK_SET failed !\n");
				return -1;
			}
			stream->f_seek_pos = offset;
			
			break;
		case SEEK_CUR:
			if((offset + stream->f_seek_pos) >= stream->f_size) return -1;
			stream->f_seek_pos += offset;
			
			break;
		case SEEK_END:
			if(stream->f_size + offset < 0) return -1;
			stream->f_seek_pos = (stream->f_size + offset);

			break;
	}

	return 0;
}

long sat_ftell(GFS_FILE *stream) {
	if (stream == NULL) return -1;

	return stream->f_seek_pos;
}

size_t sat_fread(void *ptr, size_t size, size_t nmemb, GFS_FILE *stream) {
    // Early exit conditions - combined for better branch prediction
    if (!ptr || !stream || !size || !nmemb) return 0;
    
    // Pre-calculate frequently used values
    const Uint32 seek_pos = stream->f_seek_pos;
    const Uint32 request_size = nmemb * size;
    const Uint32 remaining = stream->f_size - seek_pos;
    const Uint32 data_to_read = (request_size < remaining) ? request_size : remaining;
    
    // Early exit if no data to read
    if (!data_to_read) return 0;
    
    // Cache hit check - most common path first
    if (stream->file_hash == current_cached) {
        const Uint32 cache_end = cache_offset + CACHE_SIZE;
        
        // Fast path: data completely within cache
        if (seek_pos >= cache_offset && (seek_pos + data_to_read) <= cache_end) {
            const Uint32 offset_in_cache = seek_pos - cache_offset;
            memcpy(ptr, cache + offset_in_cache, data_to_read);
            stream->f_seek_pos = seek_pos + data_to_read;
            return data_to_read;
        }
        
        // Cache miss - reload cache
        if (data_to_read < CACHE_SIZE) {
            const Uint32 start_sector = seek_pos / SECTOR_SIZE;
            const Uint32 tot_sectors = GFS_ByteToSct(stream->fid, CACHE_SIZE);
//emu_printf("read1 %d\n", start_sector);
			GFS_Seek(stream->fid, start_sector, GFS_SEEK_SET);
            GFS_Fread(stream->fid, tot_sectors, cache, CACHE_SIZE);
            cache_offset = start_sector * SECTOR_SIZE;
            
            // Retry cache hit after reload
            const Uint32 offset_in_cache = seek_pos - cache_offset;
            memcpy(ptr, cache + offset_in_cache, data_to_read);
            stream->f_seek_pos = seek_pos + data_to_read;
            return data_to_read;
        }
    }
    
    // Direct read for large requests or cache misses
    const Uint32 start_sector = seek_pos / SECTOR_SIZE;
    const Uint32 skip_bytes = seek_pos & (SECTOR_SIZE - 1); // Use bitwise AND instead of modulo
//emu_printf("start_sector %d skip_bytes %d %p\n",start_sector,skip_bytes,current_lwram);
    // Special sector handling (consolidated check)
    static const Uint32 special_sectors[] = {3093, 3115, 3163, 3223, 3272};
    for (int i = 0; i < 5; i++) {
        if (start_sector == special_sectors[i]) {
            GFS_Seek(stream->fid, start_sector, GFS_SEEK_SET);
            break;
        }
    }
    const Sint32 tot_bytes = data_to_read + skip_bytes;
    const Sint32 tot_sectors = GFS_ByteToSct(stream->fid, tot_bytes);
    if (tot_sectors < 0) return 0;
    
    Uint32 bytes_read;
    
    if (skip_bytes) {
        // Use aligned buffer for better performance
        Uint8 *read_buffer = (Uint8*)current_lwram;
//emu_printf("read2 %d\n", start_sector);
        bytes_read = GFS_Fread(stream->fid, tot_sectors, read_buffer, tot_bytes);
        
        memcpy(ptr, read_buffer + skip_bytes, data_to_read);
        bytes_read = (bytes_read > skip_bytes) ? bytes_read - skip_bytes : 0;
    } else {
        // Direct read - no intermediate buffer needed
//emu_printf("read3\n");
        bytes_read = GFS_Fread(stream->fid, tot_sectors, ptr, tot_bytes);
        bytes_read = (bytes_read < data_to_read) ? bytes_read : data_to_read;
    }
    
    stream->f_seek_pos = seek_pos + bytes_read;
    return bytes_read;
}

int sat_feof(GFS_FILE *stream) {
	if((stream->f_size - 1) <= stream->f_seek_pos) return 1;
	else return 0;
}

// We won't ever write on CD... this is a dummy placeholder. 
size_t sat_fwrite(const void *ptr, size_t size, size_t nmemb, GFS_FILE *stream) {
	return 0;
}

int sat_ferror(GFS_FILE *stream) {
	return 0;
}

