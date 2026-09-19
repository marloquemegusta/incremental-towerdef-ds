#ifndef AUDIO_H
#define AUDIO_H

#include <nds.h>

// Inicializar el subsistema de sonido de la Nintendo DS (soundEnable)
void audio_init(void);

// Reproducir disparo visceral (Opción 3)
// pan: 0 (izquierda) a 127 (derecha), 64 = centro
void audio_play_shot(int pan);

#endif // AUDIO_H
