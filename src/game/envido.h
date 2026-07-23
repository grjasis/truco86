#ifndef TRUCO86_ENVIDO_H
#define TRUCO86_ENVIDO_H

#include "deck.h"

/* Calcula los puntos de envido de una mano de 3 cartas.
 * Si hay 2 o 3 cartas del mismo palo: 20 + suma de los valores de envido
 * (figuras=0) de las DOS mas altas de ese palo (si hay 3 del mismo palo se
 * toman las 2 mas altas igual, la 3ra no suma).
 * Si no hay ningun par del mismo palo: el valor de envido mas alto de las
 * 3 cartas sueltas (0..7). */
unsigned char envido_score(const Card hand[3]);

/* Valor de envido de una sola carta (0 para figuras 10/11/12, si no el numero). */
unsigned char envido_card_value(Card c);

#endif
