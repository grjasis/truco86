#ifndef TRUCO86_MUSIC_H
#define TRUCO86_MUSIC_H

/* Musica minima de un solo canal (pulso 2 de la APU, $4004-$4007) para la
 * pantalla de titulo: NO toca el pulso 1 (platform/sound.c), que sigue
 * libre para los SFX de todo el juego. Es una melodia corta en loop de
 * "La Cumparsita" (Gerardo Matos Rodriguez, 1916/1917, murio en 1948: de
 * dominio publico hace mas de 70 anios), transcripta nota por nota (y
 * corchea por corchea) de la notacion RTTTL del ringtone clasico de Nokia
 * del tema (ver docs/GRAFICOS.md para el detalle), bajada una octava para
 * el canal de pulso del NES. */

/* Prende el canal y arranca la melodia desde el principio. */
void music_init(void);

/* Avanza la melodia un frame: hay que llamarla una vez por vuelta del
 * loop de la pantalla de titulo (despues de wait_vblank()) para que se
 * escuche. Al llegar al final del loop, vuelve a empezar. */
void music_update(void);

/* Silencia el canal (llamar al salir de la pantalla de titulo, antes de
 * que empiece a sonar cualquier SFX del juego). */
void music_stop(void);

/* Jingle corto de un solo disparo (no en loop, ~75 cuadros = ~1.5s a
 * 50 cuadros/seg PAL-N) por el pulso 2, para cuando se termina de ganar o
 * perder una mano: bloquea hasta que termina de sonar (llamarla en vez de
 * esperar con wait_frames(), ver main.c). Al volver, el canal queda
 * silenciado y el pulso 1 (SFX) sigue disponible como siempre. */
void music_play_win(void);
void music_play_lose(void);

#endif
