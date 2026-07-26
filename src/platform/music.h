#ifndef TRUCO86_MUSIC_H
#define TRUCO86_MUSIC_H

/* Musica de la pantalla de titulo: la parte A de "La Cumparsita" (Gerardo
 * Matos Rodriguez, 1916/1917, murio en 1948: de dominio publico hace mas de
 * 70 anios) en loop, transcripta nota por nota de un arreglo en Sol menor
 * del tango (ver el detalle en music.c y en docs/GRAFICOS.md).
 *
 * Usa dos canales: la MELODIA va por el pulso 2 ($4004-$4007) y el BAJO de
 * tango (marcato de negras, fundamental y quinta) por el TRIANGULO
 * ($4008-$400B). No toca el pulso 1 ni el canal de ruido, que son de
 * platform/sound.c y siguen libres para los SFX de todo el juego. */

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
 * 50 cuadros/seg PAL-N) por el pulso 2 — un arpegio de Sol menor, la misma
 * tonalidad del tango — para cuando se termina de ganar o perder una mano:
 * bloquea hasta que termina de sonar (llamarla en vez de esperar con
 * wait_frames(), ver main.c). Al volver, el canal queda silenciado y los
 * canales de SFX siguen disponibles como siempre. */
void music_play_win(void);
void music_play_lose(void);

#endif
