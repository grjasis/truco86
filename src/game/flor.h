#ifndef TRUCO86_FLOR_H
#define TRUCO86_FLOR_H

#include "deck.h"

/* Devuelve 1 si las 3 cartas son del mismo palo (flor), 0 si no. */
unsigned char flor_has(const Card hand[3]);

/* Valor de la flor: 20 + suma de los 3 valores de envido (solo valido si
 * flor_has() es 1). */
unsigned char flor_score(const Card hand[3]);

#endif
