#pragma once

#include <stdint.h>

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
    int slot;
    pcm_sample_b_t bit;
} pcm_sample_t;

void pcm_prepare_sample(pcm_sample_t *s, size_t sz);
void pcm_sample_set_samplerate(pcm_sample_t *pcm, uint32_t sr);
void pcm_sample_start(pcm_sample_t *pcm);
void pcm_sample_stop(pcm_sample_t *pcm);
void pcm_sample_set_loop(pcm_sample_t *pcm, pcm_sample_loop_t loop);

void fm_test();