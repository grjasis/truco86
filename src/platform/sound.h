#ifndef TRUCO86_SOUND_H
#define TRUCO86_SOUND_H

/* Efectos de sonido (Fase 9 + pulido): "tira y olvida" — se dispara un
 * efecto con su propio contador de duracion en el hardware, no hace falta
 * actualizar nada cuadro a cuadro. Los de tono van por el pulso 1 (con
 * envelope y barrido, ver sound.c) y los de golpe por el canal de ruido; el
 * pulso 2 y el triangulo son de la musica (platform/music.c), asi que un
 * SFX nunca pisa la melodia ni el bajo. */
typedef enum {
    SFX_MOVE,     /* mover el cursor (menu o mano) */
    SFX_CONFIRM,  /* confirmar una opcion de menu */
    SFX_CARD,     /* jugar una carta */
    SFX_CANTO,    /* cantar o responder un envido/truco */
    SFX_WIN,      /* se ganan puntos */
    SFX_LOSE,     /* se pierden puntos */
    SFX_DEAL      /* repartir una mano nueva */
} SfxId;

/* Canales de la APU habilitados en $4015. Se escriben SIEMPRE completos (no
 * con |= ni &=): $4015 se lee y se escribe con significados distintos, asi
 * que un leer-modificar-escribir corrompe que canales quedan prendidos. */
#define APU_CH_SFX   0x09 /* pulso 1 + ruido: los dos canales de sound.c */
#define APU_CH_MUSIC 0x0F /* + pulso 2 (melodia) y triangulo (bajo), ver music.c */

/* Prende los canales de SFX. Llamar una sola vez al arrancar. */
void sound_init(void);

/* Dispara el efecto (corta el que estuviera sonando en ese canal, si habia). */
void sound_play(SfxId id);

#endif
