#include "audio.h"
#include "sound_data.h"

void audio_init(void) {
    // Activa el hardware de sonido de la Nintendo DS (canales 0-15)
    soundEnable();
}

void audio_play_shot(int pan) {
    if (pan < 0) pan = 0;
    if (pan > 127) pan = 127;
    
    // soundPlaySample(data, format, data_bytes, freq, volume, panning, loop, loopPoint)
    // SoundFormat_8Bit = PCM 8-bit con signo
    // Volumen máximo: 127
    // loop = false
    soundPlaySample(sfx_shot_visceral_pcm8,
                    SoundFormat_8Bit,
                    SFX_SHOT_VISCERAL_SIZE,
                    SFX_SHOT_VISCERAL_FREQ,
                    127,
                    pan,
                    false,
                    0);
}
