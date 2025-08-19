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
extern Sint32  gfcd_fatal_err;
extern GfsMng   *gfs_mng_ptr;
}

#include "saturn_print.h"
#define MNG_SVR(mng)            ((mng)->svr)
#define SVR_NFILE(svr)          ((svr)->nfile)
#define GFS_FILE_USED(file)     ((file)->used)
#define MNG_FILE(mng)           ((mng)->file)
//#define CACHE_SIZE (SECTOR_SIZE * 20)
#define CACHE_SIZE (SECTOR_SIZE * TOT_SECTOR)

static char satpath[25];
//static char current_path[15][16];
//static char current_path[3][16];

//static Uint32 current_cached = 0;
static Uint8 cache[CACHE_SIZE] __attribute__ ((aligned (4)));

static Uint32 cache_offset = 0;
// Used for initialization and such

void errGfsFunc(void *obj, int ec)
{
	char texte[50];
	sprintf(texte, "ErrGfs %X %X",obj, ec); 
	
	texte[49]='\0';

	emu_printf("%s\n", texte);

}
#define GFS_LOCAL
#define GFCD_ERR_OK             0       /* æ­£å¸¸çµ‚äº† */
#define GFCD_ERR_WAIT           -1      /* å‡¦ç†å¾…ã¡ */
#define GFCD_ERR_NOCDBLK        -2      /* CDãƒ–ãƒ­ãƒƒã‚¯ãŒæŽ¥ç¶šã•ã‚Œã¦ã„ãªã„ */
#define GFCD_ERR_NOFILT         -3      /* ç©ºãçµžã‚ŠãŒãªã„ */
#define GFCD_ERR_NOBUF          -4      /* ç©ºãåŒºç”»ãŒãªã„ */
#define GFCD_ERR_INUSE          -5      /* æŒ‡å®šã•ã‚ŒãŸè³‡æºãŒä½¿ç”¨ä¸­ */
#define GFCD_ERR_RANGE          -6      /* å¼•æ•°ãŒç¯„å›²å¤– */
#define GFCD_ERR_UNUSE          -7      /* æœªç¢ºä¿ã®ã‚‚ã®ã‚’æ“ä½œã—ã‚ˆã†ã¨ã—ãŸ */
#define GFCD_ERR_QFULL          -8      /* ã‚³ãƒžãƒ³ãƒ‰ã‚­ãƒ¥ãƒ¼ãŒã„ã£ã±ã„ */
#define GFCD_ERR_NOTOWNER       -9      /* éžæ‰€æœ‰è€…ãŒè³‡æºã‚’æ“ä½œã—ã‚ˆã†ã¨ã—ãŸ */
#define GFCD_ERR_CDC            -10     /* CDCã‹ã‚‰ã®ã‚¨ãƒ©ãƒ¼ */
#define GFCD_ERR_CDBFS          -11     /* CDãƒ–ãƒ­ãƒƒã‚¯ãƒ•ã‚¡ã‚¤ãƒ«ã‚·ã‚¹ãƒ†ãƒ ã‚¨ãƒ©ãƒ¼ */
#define GFCD_ERR_TMOUT          -12     /* ã‚¿ã‚¤ãƒ ã‚¢ã‚¦ãƒˆ */
#define GFCD_ERR_OPEN           -13     /* ãƒˆãƒ¬ã‚¤ãŒé–‹ã„ã¦ã„ã‚‹ */
#define GFCD_ERR_NODISC         -14     /* ãƒ‡ã‚£ã‚¹ã‚¯ãŒå…¥ã£ã¦ã„ãªã„ */
#define GFCD_ERR_CDROM          -15     /* CD-ROMã§ãªã„ãƒ‡ã‚£ã‚¹ã‚¯ãŒå…¥ã£ã¦ã„ã‚‹ */
#define GFCD_ERR_FATAL          -16     /* ã‚¹ãƒ†ãƒ¼ã‚¿ã‚¹ãŒFATAL */

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
/*
	char *path_token = (char*)strtok(satpath, "/");
emu_printf("name %s\n",path_token);
	for (idx = 0; idx < strlen(satpath); idx++)
		satpath[idx] = toupper(satpath[idx]);
emu_printf("name %s\n",path_token);
*/
	GfsHn fid = NULL;
	// OPEN FILE
	fid = GFS_Open(GFS_NameToId((Sint8*)satpath));
	
	if(fid != NULL) { // Opened!
		Sint32 fsize;
		GFS_SetTmode(fid, GFS_TMODE_SCU); // DMA transfer by SCU

		// Encapsulate the file data
		fp->fid = fid;
		GFS_GetFileInfo(fid, NULL, NULL, &fsize, NULL);
		fp->f_size = fsize;
//		emu_printf("reopen position %d %p pos %d\n", position, fp->fid, fp->f_seek_pos);
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
/*
long sat_ftell(GFS_FILE *stream) {
	if (stream == NULL) return -1;

	return stream->f_seek_pos;
}
*/
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
			memcpy(ptr, cache + offset_in_cache, dataToRead);
			stream->f_seek_pos += dataToRead;
			return dataToRead;
		} 
	
		else if ((((stream->f_seek_pos + dataToRead) >= end_offset) || (stream->f_seek_pos < cache_offset))) {
//emu_printf("cache 0x%.8X - 0x%.8X req 0x%.8X\n", cache_offset, end_offset, stream->f_seek_pos);
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
//emu_printf("skip bytes\n");
		read_buffer = (Uint8*)current_lwram;
		readBytes = GFS_Fread(stream->fid, tot_sectors, read_buffer, tot_bytes);
		memcpy(ptr, read_buffer + skip_bytes, readBytes - skip_bytes);
	} else {
//emu_printf("no skip bytes\n");
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

