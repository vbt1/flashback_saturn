#define NUM_SFXS 66
#define PCM_VOICE 18
#pragma once

#include <stdint.h>
#include <stddef.h>

struct SoundFx {
//	uint32_t offset;
	uint16_t len;
	uint16_t freq;
	uint8_t *data;
//	int8_t peak;
};

typedef enum
{
    pcm_sample_loop_no_loop = 0,
    pcm_sample_loop_loop = 1
} pcm_sample_loop_t;

typedef enum
{
    pcm_sample_16bit = 2,
    pcm_sample_8bit = 1
} pcm_sample_b_t;

typedef struct
{
    uint32_t addr;
//    int slot;
	uint8_t vol;
    pcm_sample_b_t bit;
} pcm_sample_t;

void pcm_prepare_sample(pcm_sample_t *s, uint8_t chan, size_t sz);
void pcm_sample_set_samplerate(uint8_t chan, uint32_t sr);
void pcm_sample_start(uint8_t chan);
void pcm_sample_stop(uint8_t chan);
void pcm_sample_set_loop(uint8_t chan, pcm_sample_loop_t loop);
void pcm_play(uint8_t chan, SoundFx *sfx, uint8_t volume, pcm_sample_loop_t loop);

void fm_test();