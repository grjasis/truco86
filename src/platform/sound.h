#ifndef TRUCO86_SOUND_H
#define TRUCO86_SOUND_H

/* Efectos de sonido simples (Fase 9): un solo canal de pulso, "tira y
 * olvida" (se pone un tono con contador de duracion propio del hardware,
 * no hace falta actualizar nada cuadro a cuadro). No hay musica, solo
 * SFX cortos para las acciones principales del juego. */
typedef enum {
    SFX_MOVE,     /* mover el cursor (menu o mano) */
    SFX_CONFIRM,  /* confirmar una opcion de menu */
    SFX_CARD,     /* jugar una carta */
    SFX_CANTO,    /* cantar o responder un envido/truco */
    SFX_WIN,      /* se ganan puntos */
    SFX_LOSE      /* se pierden puntos */
} SfxId;

/* Prende el canal de pulso 1. Llamar una sola vez al arrancar. */
void sound_init(void);

/* Dispara el efecto (corta el que estuviera sonando en el canal, si habia). */
void sound_play(SfxId id);

#endif
