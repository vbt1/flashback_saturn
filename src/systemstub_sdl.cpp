/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
//#define SLAVE_SOUND 1
//#define SOUND 1
extern "C" {
#include <sl_def.h>
#include <string.h>	
#include <sgl.h>

//#include <sega_mem.h>
#include <sega_int.h>
#ifdef SOUND
#include <sega_pcm.h>
#endif
//#include <sega_snd.h>
//#include "sega_csh.h"
//#include "sega_spr.h"
#include <sega_sys.h>
#include "gfs_wrap.h"
#include "sat_mem_checker.h"
#include "cdtoc.h"
void	*malloc(size_t);
}
extern void snd_init();
//extern void emu_printf(const char *format, ...);

#include "sys.h"
#include "mixer.h"
#include "systemstub.h"

 #include "saturn_print.h"

#undef assert
#define assert(x) if(!(x)){emu_printf("assert %s %d %s\n", __FILE__,__LINE__,__func__);}
/*
#undef VDP2_VRAM_A0
#define VDP2_VRAM_A0 NULL 
#undef VDP2_VRAM_B0
#define VDP2_VRAM_B0 NULL 
*/
#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
/* Needed to unlock cd drive */
#define SYS_CDINIT1(i) ((**(void(**)(int))0x60002dc)(i)) // Init functions for Saturn CD drive
#define SYS_CDINIT2() ((**(void(**)(void))0x600029c)())

/* Needed for audio */
#define SND_BUFFER_SIZE (4096)
#define SND_BUF_SLOTS 1

/* Needed for video */
#define TVSTAT	(*(Uint16 *)0x25F80004)
#define CRAM_BANK 0x5f00000 // Beginning of color ram memory addresses
#define BACK_COL_ADR (VDP2_VRAM_A1 + 0x1fffe) // Address for background colour
//#define LOW_WORK_RAM 0x00200000 // Beginning of LOW WORK RAM (1Mb)
//#define LOW_WORK_RAM_SIZE 0x100000

#ifdef _352_CLOCK_
#define HOR_OFFSET (-29.0)
#define HOR_OFFSET (0.0)// vbt pas d'offset
#else
#define HOR_OFFSET (-30.0)
#define HOR_OFFSET (0.0) // vbt pas d'offset
#endif

/* Input devices */
#define MAX_INPUT_DEVICES 1

#define PAD_PUSH_UP    (!(push & PER_DGT_KU))
#define PAD_PUSH_DOWN  (!(push & PER_DGT_KD))
#define PAD_PUSH_LEFT  (!(push & PER_DGT_KL))
#define PAD_PUSH_RIGHT (!(push & PER_DGT_KR))
#define PAD_PUSH_A  (!(push & PER_DGT_TA))
#define PAD_PUSH_B  (!(push & PER_DGT_TB))
#define PAD_PUSH_C  (!(push & PER_DGT_TC))
#define PAD_PUSH_X  (!(push & PER_DGT_TX))
#define PAD_PUSH_Z  (!(push & PER_DGT_TZ))
#define PAD_PUSH_LTRIG  (!(push & PER_DGT_TL))
#define PAD_PUSH_RTRIG  (!(push & PER_DGT_TR))
#define PAD_PUSH_START (!(push & PER_DGT_ST))

#define PAD_PULL_UP    (!(pull & PER_DGT_KU))
#define PAD_PULL_DOWN  (!(pull & PER_DGT_KD))
#define PAD_PULL_LEFT  (!(pull & PER_DGT_KL))
#define PAD_PULL_RIGHT (!(pull & PER_DGT_KR))
#define PAD_PULL_A  (!(pull & PER_DGT_TA))
#define PAD_PULL_B  (!(pull & PER_DGT_TB))
#define PAD_PULL_C  (!(pull & PER_DGT_TC))
#define PAD_PULL_X  (!(pull & PER_DGT_TX))
#define PAD_PULL_Z  (!(pull & PER_DGT_TZ))
#define PAD_PULL_LTRIG  (!(pull & PER_DGT_TL))
#define PAD_PULL_RTRIG  (!(pull & PER_DGT_TR))
#define PAD_PULL_START (!(pull & PER_DGT_ST))

typedef struct {
	volatile Uint8 access;
} SatMutex;

typedef struct {
	uint16 x, y;
	uint16 w, h;
} SAT_Rect;

/* Required for audio sound buffers */
Uint8 buffer_filled[2];
Uint8 ring_bufs[2][SND_BUFFER_SIZE * SND_BUF_SLOTS];
#ifdef SOUND
static PcmWork pcm_work[2];
static PcmHn pcm[2];
#endif
Uint8 curBuf = 0;
Uint8 curSlot = 0;
static Mixer *mix = NULL;
static volatile Uint8 audioEnabled = 1;

/* CDDA */

CDTableOfContents toc;
//CdcPly	playdata;
//CdcPos	posdata;
CdcStat statdata;

static uint8 tickPerVblank = 0;

/* Required for timing */
static SystemStub *sys = NULL;
static volatile Uint32 ticker = 0;
static volatile	Uint8  tick_wrap = 0;

/* FUNCTIONS */
#ifdef SOUND
static Uint8 firstSoundRun = 1;
static Uint8 runningSlave = 0;
static PcmHn createHandle(int bufno);
static void play_manage_buffers(void);
static void fill_buffer_slot(void);
void fill_play_audio(void);
void sat_restart_audio(void);
#endif
void vblIn(void); // This is run at each vblnk-in
uint8 isNTSC(void);

/* SDL WRAPPER */
struct SystemStub_SDL : SystemStub {
	enum {
		MAX_BLIT_RECTS = 200,
		SOUND_SAMPLE_RATE = 8400,
		JOYSTICK_COMMIT_VALUE = 3200
	};

	uint8 _overscanColor;
	uint16 _pal[512];
	uint16 _screenW, _screenH;

	/* Controller data */
	PerDigital *input_devices[MAX_INPUT_DEVICES];
	Uint8 connected_devices;

	virtual ~SystemStub_SDL() {}
	virtual void init(const char *title, uint16 w, uint16 h);
	virtual void destroy();
	virtual void setPaletteEntry(uint16 i, const Color *c);
	virtual void getPaletteEntry(uint16 i, Color *c);
	virtual void setOverscanColor(uint8 i);
	virtual void copyRect(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch);
	virtual void updateScreen(uint8 shakeOffset);
//	virtual void copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb);
	virtual void processEvents();
	virtual void sleep(uint32 duration);
	virtual uint32 getTimeStamp();
	virtual void initTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32 getOutputSampleRate();
//	virtual void *createMutex();
	virtual void destroyMutex(void *mutex);
	virtual void lockMutex(void *mutex);
	virtual void unlockMutex(void *mutex);
	virtual void setup_input (void); // Setup input controllers

	virtual void setPalette(uint8 *palette, uint16 colors);
	void prepareGfxMode();
	void cleanupGfxMode();
	void forceGfxRedraw();
	void drawRect(SAT_Rect *rect, uint8 color, uint16 *dst, uint16 dstPitch);

	void load_audio_driver(void);
	void init_cdda(void);
	void sound_external_audio_enable(uint8_t vol_l, uint8_t vol_r);
};

SystemStub *SystemStub_SDL_create() {
	sys = new SystemStub_SDL();
	return sys;
}

void SystemStub_SDL::init(const char *title, uint16 w, uint16 h) {
	memset(&_pi, 0, sizeof(_pi)); // Clean inout
//slPrint((char *)"load_audio_driver     ",slLocate(10,12));
	load_audio_driver(); // Load M68K audio driver
//slPrint((char *)"prepareGfxMode     ",slLocate(10,12));
	init_cdda();
	sound_external_audio_enable(5, 5);
	prepareGfxMode(); // Prepare graphic output
//		//emu_printf("prepareGfxMode\n");	
//slPrint((char *)"setup_input     ",slLocate(10,12));
	setup_input(); // Setup controller inputs

	memset(_pal, 0, sizeof(_pal));

	audioEnabled = 0;
	curBuf = 0;
	curSlot = 0;
//emu_printf("SystemStub_SDL::init\n");	
#ifdef SLAVE_SOUND
	*(Uint8*)OPEN_CSH_VAR(buffer_filled[0]) = 0;
	*(Uint8*)OPEN_CSH_VAR(buffer_filled[1]) = 0;
#else
	buffer_filled[0] = 0;
	buffer_filled[1] = 0;
#endif
	if(isNTSC())
		tickPerVblank = 17;
	else
		tickPerVblank = 20;

	slIntFunction(vblIn); // Function to call at each vblank-in // vbt à remettre

	return;
}

void SystemStub_SDL::destroy() {
	cleanupGfxMode();
	SYS_Exit(0);
}

void SystemStub_SDL::setPalette(uint8 *palette, uint16 colors) {
	assert(colors <= 256);
	for (int i = 0; i < colors; ++i) {
		uint8 r = palette[i * 3];
		uint8 g = palette[i * 3 + 1];
		uint8 b = palette[i * 3 + 2];

		_pal[i] = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) | RGB_Flag; // BGR for saturn
	}
	
}

void SystemStub_SDL::setPaletteEntry(uint16 i, const Color *c) {
	uint8 r = (c->r << 2) | (c->r & 3);
	uint8 g = (c->g << 2) | (c->g & 3);
	uint8 b = (c->b << 2) | (c->b & 3);

	_pal[i] = ((b >> 3) << 10) | ((g >> 3) << 5) | ((r >> 3) << 0) | RGB_Flag;
}

void SystemStub_SDL::getPaletteEntry(uint16 i, Color *c) {
	Uint8 b = ((_pal[i] >> 10) & 0x1F) << 1;
	Uint8 g = ((_pal[i] >> 5)  & 0x1F) << 1;
	Uint8 r = ((_pal[i] >> 0)  & 0x1F) << 1;

	c->r = r | 1;
	c->g = g | 1;
	c->b = b | 1;
}

void SystemStub_SDL::setOverscanColor(uint8 i) {
	_overscanColor = i;
	memset((void*)VDP2_VRAM_A0, i, 512*448);
}

void SystemStub_SDL::copyRect(int16 x, int16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) {
	// Calculate initial source and destination pointers
	uint8 *srcPtr = (uint8 *)(buf + y * pitch + x);
	uint8 *dstPtr = (uint8 *)(VDP2_VRAM_A0 + y * pitch + x);

	if (x == 0) {
//		memcpyl(dstPtr, srcPtr, w * h);
		DMA_ScuMemCopy(dstPtr, srcPtr, w * h);
	} else {
		for (uint16 idx = 0; idx < h; ++idx) {
			DMA_ScuMemCopy(dstPtr, srcPtr, w);
			srcPtr += pitch;
			dstPtr += pitch;
		}
	}
}

void SystemStub_SDL::updateScreen(uint8 shakeOffset) {
	slTransferEntry((void*)_pal, (void*)(CRAM_BANK + 512), 256 * 4);  // vbt à remettre
//	memcpy((void*)(CRAM_BANK + 512), (void*)_pal, 256 * 4);  // vbt à remettre
}

void SystemStub_SDL::processEvents() {
	Uint16 push;
	Uint16 pull;

	switch(input_devices[0]->id) { // Check only the first controller...
		case PER_ID_StnAnalog: // ANALOG PAD
		case PER_ID_StnPad: // DIGITAL PAD 
			_pi.lastChar = 0;

			push = (volatile Uint16)(input_devices[0]->push);
			pull = (volatile Uint16)(input_devices[0]->pull);

			if (PAD_PULL_UP)
				_pi.dirMask &= ~PlayerInput::DIR_UP;
			else if (PAD_PUSH_UP)
				_pi.dirMask |= PlayerInput::DIR_UP;

			if (PAD_PULL_DOWN)
				_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			else if (PAD_PUSH_DOWN)
				_pi.dirMask |= PlayerInput::DIR_DOWN;

			if (PAD_PULL_LEFT)
				_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			else if (PAD_PUSH_LEFT)
				_pi.dirMask |= PlayerInput::DIR_LEFT;

			if (PAD_PULL_RIGHT)
				_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			else if (PAD_PUSH_RIGHT)
				_pi.dirMask |= PlayerInput::DIR_RIGHT;

			if (PAD_PULL_START)
				_pi.enter = false;
			else if (PAD_PUSH_START)
				_pi.enter = true;

			if (PAD_PULL_B)
				_pi.space = false;
			else if (PAD_PUSH_B)
				_pi.space = true;

			if (PAD_PULL_X)
				_pi.backspace = false;
			else if (PAD_PUSH_X)
				_pi.backspace = true;

			if (PAD_PULL_A)
				_pi.shift = false;
			else if (PAD_PUSH_A)
				_pi.shift = true;

			if (PAD_PULL_LTRIG)
				//_pi.ltrig = false;
				;
			else if (PAD_PUSH_LTRIG)
				_pi.ltrig = true;

			if (PAD_PULL_RTRIG)
				//_pi.rtrig = false;
				;
			else if (PAD_PUSH_RTRIG)
				_pi.rtrig = true;

			if (PAD_PUSH_Z)
				_pi.escape = true;

			break;
		default:
			break;
	}

	return;
}

void SystemStub_SDL::sleep(uint32 duration) {
	static Uint8 counter = 0;

	uint32 wait_tick = ticker + duration;
	counter++;

	while(wait_tick >= ticker);
}

uint32 SystemStub_SDL::getTimeStamp() {
	return ticker;
}

void SystemStub_SDL::initTimeStamp() {
	ticker = 0;
}

void SystemStub_SDL::startAudio(AudioCallback callback, void *param) {
#ifdef SOUND
	mix = (Mixer*)param;

	memset(ring_bufs, 0, SND_BUFFER_SIZE * 2 * SND_BUF_SLOTS);

	PCM_Init(); // Initialize PCM playback

	audioEnabled = 1; // Enable audio

	// Prepare handles
	pcm[0] = createHandle(0);
	pcm[1] = createHandle(1);

	// start playing
	PCM_Start(pcm[0]); 
	PCM_EntryNext(pcm[1]);
#endif
}

void SystemStub_SDL::stopAudio() {
#ifdef SOUND
	audioEnabled = 0;

	// Stopping playback
	PCM_Stop(pcm[0]);
	PCM_Stop(pcm[1]);

	// Destroy handles
	PCM_DestroyMemHandle(pcm[0]);
	PCM_DestroyMemHandle(pcm[1]);

	// Deinitialize PCM playback
	PCM_Finish();


	return;
#endif
}

uint32 SystemStub_SDL::getOutputSampleRate() {
	return SOUND_SAMPLE_RATE;
}
/*
void *SystemStub_SDL::createMutex() {
//emu_printf("SystemStub_SDL::createMutex\n");	
	SatMutex *mtx = (SatMutex*)malloc(sizeof(SatMutex));
#ifdef SLAVE_SOUND
	*(Uint8*)OPEN_CSH_VAR(mtx->access) = 0;
#else
	mtx->access = 0;
#endif
	return mtx;
}
*/
void SystemStub_SDL::destroyMutex(void *mutex) {
	sat_free(mutex);
	return;
}

void SystemStub_SDL::lockMutex(void *mutex) {
	SatMutex *mtx = (SatMutex*)mutex;
#ifdef SLAVE_SOUND	
	while(*(Uint8*)OPEN_CSH_VAR(mtx->access) > 0) asm("nop");
	(*(Uint8*)OPEN_CSH_VAR(mtx->access))++;
#else
//	while(mtx->access > 0) asm("nop");
	mtx->access++;
#endif
	return;
}

void SystemStub_SDL::unlockMutex(void *mutex) {
//emu_printf("SystemStub_SDL::unlockMutex\n");	
	SatMutex *mtx = (SatMutex*)mutex;
#ifdef SLAVE_SOUND
	(*(Uint8*)OPEN_CSH_VAR(mtx->access))--;
#else
	mtx->access--;
#endif
	return;
}

void SystemStub_SDL::prepareGfxMode() {
	slTVOff(); // Turn off display for initialization

	slColRAMMode(CRM16_2048); // Color mode: 1024 colors, choosed between 16 bit

//	slBitMapNbg1(COL_TYPE_256, BM_512x512, (void*)VDP2_VRAM_A0); // Set this scroll plane in bitmap mode
	slBMPaletteNbg0(1); // NBG1 (game screen) uses palette 1 in CRAM
	slBMPaletteNbg1(1); // NBG1 (game screen) uses palette 1 in CRAM
	slColRAMOffsetSpr(2) ;  // spr palette
#ifdef _352_CLOCK_
	// As we are using 352xYYY as resolution and not 320xYYY, this will take the game back to the original aspect ratio
#endif
	
	memset((void*)VDP2_VRAM_A0, 0x00, 512*448); // Clean the VRAM banks. // à remettre
	memset((void*)(SpriteVRAM + cgaddress),0,0x30000);
	slPriorityNbg0(4); // Game screen
	slPriorityNbg1(6); // Game screen
	slPrioritySpr0(4);
	
//	slScrPosNbg1(toFIXED(HOR_OFFSET), toFIXED(0.0)); // Position NBG1, offset it a bit to center the image on a TV set

	slScrTransparent(NBG0ON); // Do NOT elaborate transparency on NBG1 scroll
//	slZoomNbg0(toFIXED(0.8), toFIXED(1.0));
//	slZoomNbg1(toFIXED(0.8), toFIXED(1.0));


	slZdspLevel(7); // vbt : ne pas d?placer !!!
	slBack1ColSet((void *)BACK_COL_ADR, 0x8000); // Black color background
	
	extern Uint16 VDP2_RAMCTL;	
	VDP2_RAMCTL = VDP2_RAMCTL & 0xFCFF;
	extern Uint16 VDP2_TVMD;
	VDP2_TVMD &= 0xFEFF;
	slScrAutoDisp(NBG0ON|NBG1ON|SPRON); // à faire toujours en dernier
	slScrCycleSet(0x55EEEEEE , NULL , 0x44EEEEEE , NULL);
	slWindow(63 , 0 , 574 , 447 , 241 ,320 , 224);

	SPRITE *sys_clip = (SPRITE *) SpriteVRAM;
	(*sys_clip).XC = 574;

	slScrWindow0(63 , 0 , 574 , 447 );
	slScrWindowModeNbg0(win0_IN);
	slScrWindow1(63 , 0 , 574 , 447 );
	slScrWindowModeNbg1(win1_IN);
	slScrWindowModeSPR(win0_IN);
	
	slScrPosNbg0(toFIXED(-63),0) ;
	slScrPosNbg1(toFIXED(-63),0) ;
	slSpecialPrioModeNbg0(spPRI_Dot);
	slSpecialPrioBitNbg0(1); // obligatoire
	slSpecialFuncCodeA(sfCOL_ef);
//	slSpecialFuncCodeB(0x4);
	slTVOn(); // Initialization completed... tv back on
	slSynch();  // faire un slsynch à la fin de la config
	return;
}

void SystemStub_SDL::cleanupGfxMode() {
	slTVOff();
	return;
}

void SystemStub_SDL::forceGfxRedraw() {
	return;
}

void SystemStub_SDL::drawRect(SAT_Rect *rect, uint8 color, uint16 *dst, uint16 dstPitch) {
	return;
}
// Store the info on connected peripheals inside an array
void SystemStub_SDL::setup_input (void) {
	if ((Per_Connect1 + Per_Connect2) == 0) {
		connected_devices = 0;
		return; // Nothing connected...
	}
	
	Uint8 index, input_index = 0;

	// check up to 6 peripheals on left connector
	for(index = 0; (index < Per_Connect1) && (input_index < MAX_INPUT_DEVICES); index++)
		if(Smpc_Peripheral[index].id != PER_ID_NotConnect) {
			input_devices[input_index] = &(Smpc_Peripheral[index]);
			input_index++;
		}

	// check up to 6 peripheals on right connector 
	for(index = 0; (index < Per_Connect2) && (input_index < MAX_INPUT_DEVICES); index++)
		if(Smpc_Peripheral[index + 15].id != PER_ID_NotConnect) {
			input_devices[input_index] = &(Smpc_Peripheral[index + 15]);
			input_index++;
		}

	connected_devices = input_index;
}

void SystemStub_SDL::load_audio_driver(void) {
	snd_init();
	return;
}

void SystemStub_SDL::init_cdda(void)
{
	CdcPly playdata;
	CDC_TgetToc((Uint32*)&toc);
	
    CDC_PLY_STYPE(&playdata) = CDC_PTYPE_TNO;	/* set by track number.*/
    CDC_PLY_STNO( &playdata) = 2;		/* start track number. */
    CDC_PLY_SIDX( &playdata) = 1;		/* start index number. */
    CDC_PLY_ETYPE(&playdata) = CDC_PTYPE_TNO;	/* set by track number.*/
    CDC_PLY_ETNO( &playdata) = 48;		/* end track number. */
    CDC_PLY_EIDX( &playdata) = 99;		/* start index number. */
    CDC_PLY_PMODE(&playdata) = CDC_PTYPE_NOCHG;//CDC_PM_DFL + 30;	/* Play Mode. */ // lecture en boucle
//    CDC_PLY_PMODE(&playdata) = CDC_PTYPE_NOCHG;//CDC_PM_DFL+30;//CDC_PM_DFL ;	/* Play Mode. */ // lecture unique
	statdata.report.fad = 0;
	
}

 void SystemStub_SDL::sound_external_audio_enable(uint8_t vol_l, uint8_t vol_r) {
    volatile uint16_t *slot_ptr;

    //max sound volume is 7
    if (vol_l > 7) {
        vol_l = 7;
    }
    if (vol_r > 7) {
        vol_r = 7;
    }

    // Setup SCSP Slot 16 and Slot 17 for playing
    slot_ptr = (volatile Uint16 *)(0x25B00000 + (0x20 * 16));
    slot_ptr[0] = 0x1000;
    slot_ptr[1] = 0x0000; 
    slot_ptr[2] = 0x0000; 
    slot_ptr[3] = 0x0000; 
    slot_ptr[4] = 0x0000; 
    slot_ptr[5] = 0x0000; 
    slot_ptr[6] = 0x00FF; 
    slot_ptr[7] = 0x0000; 
    slot_ptr[8] = 0x0000; 
    slot_ptr[9] = 0x0000; 
    slot_ptr[10] = 0x0000; 
    slot_ptr[11] = 0x001F | (vol_l << 5);
    slot_ptr[12] = 0x0000; 
    slot_ptr[13] = 0x0000; 
    slot_ptr[14] = 0x0000; 
    slot_ptr[15] = 0x0000; 

    slot_ptr = (volatile Uint16 *)(0x25B00000 + (0x20 * 17));
    slot_ptr[0] = 0x1000;
    slot_ptr[1] = 0x0000; 
    slot_ptr[2] = 0x0000; 
    slot_ptr[3] = 0x0000; 
    slot_ptr[4] = 0x0000; 
    slot_ptr[5] = 0x0000; 
    slot_ptr[6] = 0x00FF; 
    slot_ptr[7] = 0x0000; 
    slot_ptr[8] = 0x0000; 
    slot_ptr[9] = 0x0000; 
    slot_ptr[10] = 0x0000; 
    slot_ptr[11] = 0x000F | (vol_r << 5);
    slot_ptr[12] = 0x0000; 
    slot_ptr[13] = 0x0000; 
    slot_ptr[14] = 0x0000; 
    slot_ptr[15] = 0x0000;

    *((volatile Uint16 *)(0x25B00400)) = 0x020F;
}

inline void timeTick() {
	if(ticker > (0xFFFFFFFF - tickPerVblank)) {
		tick_wrap ^= 1;
		ticker = 0;
	} else {
		ticker += tickPerVblank;
	}
}

void vblIn (void) {
	//static Uint8 counter = 0;
//emu_printf("vblIn\n");
	// Process input
	sys->processEvents();
	sys->updateScreen(0);
	timeTick();
}

uint8 isNTSC (void) {
	if(!(TVSTAT & 1))
		return 1;
	else
		return 0;
}

/*
void SCU_DMAWait(void) {
	Uint32 res;

	while((res = DMA_ScuResult()) == 2);
	
	if(res == 1) {
		//emu_printf("SCU DMA COPY FAILED!\n");
	}
}
*/
