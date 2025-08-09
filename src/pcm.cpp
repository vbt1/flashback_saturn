extern "C"
{
#include <stdarg.h>
#include <string.h>
#include "scsp.h"
#include "pcm.h"

void emu_printf(const char *format, ...);
int32_t GFS_Load(int32_t fid, int32_t ofs, void *buf, int32_t bsize);
int32_t GFS_NameToId(int8_t *fname);
}

static const int logtbl[] = {
    /* 0 */ 0,
    /* 1 */ 1,
    /* 2 */ 2, 2,
    /* 4 */ 3, 3, 3, 3,
    /* 8 */ 4, 4, 4, 4, 4, 4, 4, 4,
    /* 16 */ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    /* 32 */ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 64 */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 128 */ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
	
#define PCM_MSK1(a)				((a)&0x0001)
#define PCM_MSK3(a)				((a)&0x0007)
#define PCM_MSK4(a)				((a)&0x000F)
#define PCM_MSK5(a)				((a)&0x001F)
#define PCM_MSK10(a)			((a)&0x03FF)

#define PCM_SCSP_FREQUENCY					(44100L)

#define PCM_CALC_OCT(smpling_rate) 											\
		((int)logtbl[PCM_SCSP_FREQUENCY / ((smpling_rate) + 1)])
		
#define PCM_CALC_SHIFT_FREQ(oct)											\
		(PCM_SCSP_FREQUENCY >> (oct))
		
#define PCM_CALC_FNS(smpling_rate, shift_freq)								\
		((((smpling_rate) - (shift_freq)) << 10) / (shift_freq))
		
#define PCM_SET_PITCH_WORD(oct, fns)										\
		((int)((PCM_MSK4(-(oct)) << 11) | PCM_MSK10(fns)))

void snd_init()
{
//	emu_printf("snd_init\n");
   *(volatile uint8_t *)(0x25B00400) = 0x02;
   // Turn off Sound CPU
   smpc_smc_sndoff_call();

   // Make sure SCSP is set to 512k mode
   *(volatile uint8_t *)(0x25B00400) = 0x02;

   // Clear Sound Ram
   for (int i = 0; i < 0x80000; i += 4)
      *(volatile uint32_t *)(0x25A00000 + i) = 0x00000000;

   // Copy driver over
//   for (int i = 0; i < sound_driver_size; i++)
//      *(volatile uint8_t *)(0x25A00000 + i) = sound_driver[i];
	GFS_Load(GFS_NameToId((int8_t *)"SDRV.BIN"),0,(void *)0x25A00000,sound_driver_size);

   // Turn on Sound CPU again
   smpc_smc_sndon_call();
   
    volatile uint16_t *control = (uint16_t *)0x25b00400;
    control[0] = 0x20f; // master vol  vbt : pour volume et 4mb   
//	emu_printf("snd_init done\n");   
}

void pcm_prepare_sample(pcm_sample_t *s, uint8_t chan, size_t sz)
{
    // fill slot
    volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(chan);
	asm("nop");

    // setup sample
    slot->pcm8b = s->bit & 1;
	
    slot->sa = s->addr-SCSP_RAM;
    slot->lsa = 0;
    slot->lea = (sz / s->bit) - 1;

//	emu_printf("s->addr-SCSP_RAM %06x slot->pcm8b %d sz %d slot->lea %x\n",s->addr-SCSP_RAM,slot->pcm8b,sz,slot->lea);

    // why 31 ?
    slot->attack_rate = 31;
    slot->decay_l = 31;
    slot->release_r = 31;

    // slot->sdir = 1;
    // volume & pan
    slot->disdl = s->vol;
    slot->dipan = 0;
	asm("nop");
}

void pcm_sample_set_samplerate(uint8_t chan, uint32_t samplerate)
{
    volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(chan);
	asm("nop");
	
    int octr;
    int shiftr;
    int fnsr;

    octr = PCM_CALC_OCT(samplerate);
    shiftr = PCM_CALC_SHIFT_FREQ(octr);
    fnsr = PCM_CALC_FNS(samplerate, shiftr);

    // frequency
    slot->oct = (-octr) & 0xf;
    slot->fns = fnsr;
	asm("nop");

}

void pcm_sample_start(uint8_t chan)
{
    volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(chan);
	asm("nop");

    // start playback
    slot->kyonb = 1;
    slot->kyonex = 1;
	asm("nop");
//	emu_printf("slot %d addr %x len %x vol %d\n",chan,slot->sa, slot->lea+1,slot->disdl);
}

void pcm_sample_stop(uint8_t chan)
{
    volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(chan);
	asm("nop");
    // stop playback
    slot->kyonb = 0;
    slot->kyonex |= 1;
}

void pcm_sample_set_loop(uint8_t chan, pcm_sample_loop_t loop)
{
    volatile scsp_slot_regs_t *slot = (scsp_slot_regs_t *)get_scsp_slot(chan);
	asm("nop");
    slot->lpctl = loop;
	asm("nop");
}

void pcm_play(uint8_t chan, SoundFx *sfx, uint8_t volume, pcm_sample_loop_t loop)
{
	uint32_t address = (uint32_t)sfx->data;
	pcm_sample_t pcm = {.addr = address, .vol = volume, .bit = pcm_sample_8bit};
	pcm_prepare_sample(&pcm, chan, sfx->len);
	pcm_sample_set_samplerate(chan, sfx->freq);
	pcm_sample_set_loop(chan, loop);
	pcm_sample_start(chan);	
}

#define SMPC_REG_SF             *((volatile uint8_t *)0x20100063)
void smpc_wait_till_ready (void)
{
   // Wait until SF register is cleared
   while(SMPC_REG_SF & 0x1) { }
}

void smpc_smc_wait(int enter)
{
        volatile unsigned char *reg_sf;
        reg_sf = (volatile unsigned char *)SMPC(SF);

        while ((*reg_sf & 0x01) == 0x01);

        if (enter) {
                *reg_sf = 0x01;
        }
}

unsigned char smpc_smc_call(cpu_smpc_cmd_t cmd)
{
//        smpc_smc_wait(1);
		smpc_wait_till_ready();

        MEMORY_WRITE(8, SMPC(COMREG), cmd);

        smpc_smc_wait(0);

        return MEMORY_READ(8, OREG(31));
}

#define SMPC_CMD_SNDON				0x06
#define SMPC_CMD_SNDOFF				0x07
//#define SMPC_SMC_SNDOFF				0x07
/* E:\esp_saturn\loader_yaul\CANAL.VGM (14/09/2016 01:52:02)
   DébutPosition(h): 00000000, FinPosition(h): 0001E3BF, Longueur(h): 0001E3C0 */
unsigned char smpc_smc_sndoff_call(void)
{
        return smpc_smc_call(SMPC_SMC_SNDOFF);
}

unsigned char smpc_smc_sndon_call(void)
{
        return smpc_smc_call(SMPC_SMC_SNDON);
}


