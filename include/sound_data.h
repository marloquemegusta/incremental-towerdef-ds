#ifndef SOUND_DATA_H
#define SOUND_DATA_H

#include <stdint.h>
#include <stddef.h>

#define SFX_SHOT_VISCERAL_FREQ 16000
#define SFX_SHOT_VISCERAL_SIZE 3520

// Aligned 4-byte buffer for Nintendo DS hardware DMA sound channel
extern const int8_t sfx_shot_visceral_pcm8[SFX_SHOT_VISCERAL_SIZE] __attribute__((aligned(4)));

#endif // SOUND_DATA_H
