#ifndef TRUCO86_VSYNC_H
#define TRUCO86_VSYNC_H

/* Incrementado por el manejador de NMI en crt0.s en cada frame (50 veces
 * por segundo en PAL-N). No usar directamente salvo para diagnostico: usar
 * wait_vblank(). */
extern volatile unsigned char nes_frame_count;

/* Bloquea hasta el proximo NMI (inicio del siguiente frame/vblank). */
void wait_vblank(void);

#endif
