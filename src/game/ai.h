#ifndef TRUCO86_AI_H
#define TRUCO86_AI_H

#include "deck.h"

/* IA de la CPU (Fase 7): puro, sin hardware, testeable en el host. Las
 * decisiones de "aceptar/escalar" reciben un byte 'entropy' cualquiera
 * (en el juego real se le pasa el contador de frames, ver
 * platform/vsync.h) para variar un poco la decision cerca del umbral y
 * que la CPU no sea 100% predecible siempre jugando igual con la misma
 * mano. Con el mismo 'entropy' el resultado es siempre el mismo (para
 * poder testear). */

/* Elige que carta de 'hand' (evitando las que ya tengan played[i]==1) le
 * conviene jugar a la CPU en una baza, sabiendo la carta que ya jugo el
 * rival (para cuando la CPU responde, ver game/match.c y la regla de
 * "el que gana tira primero" en REGLAS.md). Si puede ganar la baza, juega
 * la carta ganadora MAS FLOJA (para guardarse las fuertes para despues);
 * si no puede ganar, sacrifica la carta mas floja que le queda. Devuelve
 * el indice (0..2) elegido. */
unsigned char ai_choose_card(const Card hand[3], const unsigned char played[3], Card opponent_card);

/* Elige que carta jugar cuando la CPU es quien ABRE la baza (gano la
 * anterior, o quedo parda): sin saber todavia la carta del rival, juega
 * la MAS FLOJA que le queda (se guarda las fuertes, tantea con la floja).
 * Devuelve el indice (0..2) elegido. */
unsigned char ai_choose_lead_card(const Card hand[3], const unsigned char played[3]);

/* Envido/flor: 1 si la CPU acepta (quiero) un canto de valor 'value' dado
 * su propio puntaje 'own_score'. */
unsigned char ai_envido_accept(unsigned char own_score, unsigned char value, unsigned char entropy);

/* Envido/flor: 1 si la CPU escala (canta de nuevo) en vez de solo aceptar. */
unsigned char ai_envido_escalate(unsigned char own_score, unsigned char entropy);

/* Truco: 1 si la CPU acepta (quiero), dado el poder de truco (ver
 * card_rank.h) de su mejor carta sin jugar todavia. */
unsigned char ai_truco_accept(unsigned char best_power, unsigned char entropy);

/* Truco: 1 si la CPU escala (canta de nuevo) en vez de solo aceptar. */
unsigned char ai_truco_escalate(unsigned char best_power, unsigned char entropy);

#endif
