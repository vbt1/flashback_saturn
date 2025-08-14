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
#define CACHE_SIZE (SECTOR_SIZE * TOT_SECTOR)

static char satpath[25];
//static char current_path[15][16];
//static char current_path[3][16];

//static Uint32 current_cached = 0;
static Uint8 cache[CACHE_SIZE] __attribute__ ((aligned (4)));

//static Uint8  fully_cached = 0; // If file is cached from start to finish
static Uint32 cache_offset = 0;
// Used for initialization and such


//Uint8 dir_depth = 0;
/*
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

*/
void errGfsFunc(void *obj, int ec)
{
	char texte[50];
	sprintf(texte, "ErrGfs %X %X",obj, ec); 
	texte[49]='\0';

//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"planté gfs",70,130);
//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)texte,70,140);
//	emu_printf("%s\n", texte);

    int      ret;
    GfsErrStat  stat;
    GFS_GetErrStat(&stat);
    ret = GFS_ERR_CODE(&stat);

	sprintf(texte, "ErrGfsCode %X",ret); 
	texte[49]='\0';

//	FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)texte,70,150);
	emu_printf("%s\n", texte);
//	wait_vblank();

	while(1);
}


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


void init_GFS() { //Initialize GFS system

	CDC_CdInit(0x00,0x00,0x05,0x0f);

    GFS_DIRTBL_TYPE(&gfsDirTbl) = GFS_DIR_NAME;
    GFS_DIRTBL_DIRNAME(&gfsDirTbl) = gfsDirName;
    GFS_DIRTBL_NDIR(&gfsDirTbl) = DIR_MAX;
    gfsDirN = GFS_Init(OPEN_MAX, gfsLibWork, &gfsDirTbl);
//	dir_depth = 0;
	GFS_SetErrFunc((GfsErrFunc)errGfsFunc, NULL );
	memset4_fast(cache, 0, CACHE_SIZE);
}

GFS_FILE *sat_fopen(const char *path, const int position) {
//slSynch();
	memset(satpath, 0, 25);

	if (path == NULL) 
	{	
		return NULL; // nothing to do...
	}
	Uint16 idx;
	static GFS_FILE fp[1];
	idx = 0;

	Uint16 path_len = strlen(path);
	strncpy(satpath, path, path_len + 1);

	char *path_token = (char*)strtok(satpath, "/");

	for (idx = 0; idx < strlen(satpath); idx++)
		satpath[idx] = toupper(satpath[idx]);

	GfsHn fid = NULL;
	// OPEN FILE
	fid = GFS_Open(GFS_NameToId((Sint8*)path_token));
	
	if(fid != NULL) { // Opened!
		Sint32 fsize;
		GFS_SetTmode(fid, GFS_TMODE_CPU); // DMA transfer by SCU

		// Encapsulate the file data
		fp->fid = fid;
		GFS_GetFileInfo(fid, NULL, NULL, &fsize, NULL);
		fp->f_size = fsize;

		if (!position) // on conserve le cache existant
		{
			fp->f_seek_pos = 0;
			Sint32 tot_sectors = TOT_SECTOR;
			GFS_Seek(fp->fid, 0, GFS_SEEK_SET);
//			memset4_fast((Uint8*)cache, 0, CACHE_SIZE);
			cache_offset = 0;
			GFS_Fread(fp->fid, tot_sectors, (Uint8*)cache, CACHE_SIZE);
		}
		else
		{
//			emu_printf("reopen position %d %p\n", position, fp->fid);
			GFS_Seek(fp->fid, position, GFS_SEEK_SET);
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
				emu_printf("SEEK_SET failed !\n");
				return -1;
			}
			stream->f_seek_pos = offset;
			
			break;
/*
		case SEEK_CUR:
			if((offset + stream->f_seek_pos) >= stream->f_size) return -1;
			stream->f_seek_pos += offset;
			
			break;
		case SEEK_END:
			if(stream->f_size + offset < 0) return -1;
			stream->f_seek_pos = (stream->f_size + offset);

			break;*/
	}

	return 0;
}

long sat_ftell(GFS_FILE *stream) {
	if (stream == NULL) return -1;

	return stream->f_seek_pos;
}

#define GFS_BYTE_SCT(byte, sctsiz)  \
    ((Sint32)(((Uint32)(byte)) + ((Uint32)(sctsiz)) - 1) / ((Uint32)(sctsiz)))

size_t sat_fread(void *ptr, size_t size, size_t nmemb, GFS_FILE *stream) {
//if(nmemb==4)
//emu_printf("sat_fread nmemb %d fsize %d\n", nmemb, stream->f_size);	

	if (ptr == NULL || stream == NULL) return 0; // nothing to do then
	if (size == 0 || nmemb == 0) return 0;

	Uint8 *read_buffer = NULL;
	Sint32 tot_bytes; // Total bytes to read
	Sint32 tot_sectors;
	Uint32 readBytes;

	Uint32 start_sector = (stream->f_seek_pos)/SECTOR_SIZE;
//	Uint32 skip_bytes = (stream->f_seek_pos)%SECTOR_SIZE; // Bytes to skip at the beginning of sector
	Uint32 skip_bytes = stream->f_seek_pos & (SECTOR_SIZE - 1);
//emu_printf("before seek fread %d  sz %d skip_bytes %d\n", start_sector, size, skip_bytes);
	if(GFS_Seek(stream->fid, start_sector, GFS_SEEK_SET) < 0) 
	{	
//	emu_printf("GFS_Seek return 0\n");
	return 0;
	}
	tot_bytes = (nmemb * size) + skip_bytes;
//emu_printf("start_sector %d stream->f_seek_pos %d\n",start_sector,stream->f_seek_pos);
//	tot_sectors = GFS_ByteToSct(stream->fid, tot_bytes);
	tot_sectors = GFS_BYTE_SCT(tot_bytes, SECTOR_SIZE);
	if(tot_sectors < 0) return 0;
	Uint32 remaining_data, request_block;
	
	remaining_data = stream->f_size - stream->f_seek_pos;
	request_block = nmemb * size;
//	cache_offset=0; // vbt : on force la mise à zéro du cache, corrige un cas d'écran incorrect
	Uint32 dataToRead = MIN(request_block, remaining_data);

	if (/*(stream->file_hash == current_cached) &&*/ ((dataToRead + skip_bytes) < CACHE_SIZE)) {
		Uint32 end_offset;

partial_cache:
		end_offset = cache_offset + CACHE_SIZE;

		if(((stream->f_seek_pos + dataToRead) < end_offset) && (stream->f_seek_pos >= cache_offset)) {
			Uint32 offset_in_cache = stream->f_seek_pos - cache_offset;

//emu_printf("offset_in_cache %d \n", offset_in_cache);			
//emu_printf("start offset %d \n", start_sector * SECTOR_SIZE);			
//emu_printf("stream->f_seek_pos %d \n", stream->f_seek_pos);
//emu_printf("cache_offset %d\n", cache_offset);
//emu_printf("stream->f_seek_pos %d cache_offset %d %x %x %x %x withoff %x %x\n",stream->f_seek_pos,cache_offset,cache[0],cache[1],cache[8],cache[9],cache[offset_in_cache+8],cache[offset_in_cache+9]);	
			memcpy(ptr, cache + offset_in_cache, dataToRead);
//emu_printf("ptr[0] %x ptr[1] %x  ptr[2] %x ptr[3] %x ptr[4] %x ptr[5] %x ptr[6] %x ptr[7] %x ptr[8] %x ptr[9] %x ptr[10] %x ptr[11] %x ptr[12] %x\n",
//((Uint8 *)ptr)[1538],((Uint8 *)ptr)[1538+1],((Uint8 *)ptr)[1538+2],((Uint8 *)ptr)[3],
//((Uint8 *)ptr)[4],((Uint8 *)ptr)[5],((Uint8 *)ptr)[6],((Uint8 *)ptr)[7],
//((Uint8 *)ptr)[8],((Uint8 *)ptr)[9],((Uint8 *)ptr)[10],((Uint8 *)ptr)[11],((Uint8 *)ptr)[11]);
			stream->f_seek_pos += dataToRead;
			return dataToRead;
		} 
	
		else if ((((stream->f_seek_pos + dataToRead) >= end_offset) || (stream->f_seek_pos < cache_offset))) {
///emu_printf("cache 0x%.8X - 0x%.8X req 0x%.8X\n", cache_offset, end_offset, stream->f_seek_pos);
			start_sector = stream->f_seek_pos / SECTOR_SIZE;
			skip_bytes = stream->f_seek_pos & (SECTOR_SIZE - 1); // Use bitwise AND instead of modulo

			tot_bytes = CACHE_SIZE;
			tot_sectors = GFS_BYTE_SCT(tot_bytes, SECTOR_SIZE);

			GFS_Seek(stream->fid, start_sector, GFS_SEEK_SET);
			readBytes = GFS_Fread(stream->fid, tot_sectors, (Uint8*)cache, tot_bytes);
			cache_offset = start_sector * SECTOR_SIZE; // obligatoire
//			readBytes = tot_bytes;
			goto partial_cache;
		}
	}

	if(skip_bytes) {
		read_buffer = (Uint8*)current_lwram;
		readBytes = GFS_Fread(stream->fid, tot_sectors, read_buffer, tot_bytes);
		memcpy(ptr, read_buffer + skip_bytes, readBytes - skip_bytes);
	} else {
		readBytes = GFS_Fread(stream->fid, tot_sectors, ptr, tot_bytes);
	}
	stream->f_seek_pos += (readBytes - skip_bytes); // Update the seek cursor 
	return (readBytes - skip_bytes);
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

