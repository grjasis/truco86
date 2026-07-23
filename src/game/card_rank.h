#ifndef TRUCO86_CARD_RANK_H
#define TRUCO86_CARD_RANK_H

#include "deck.h"

/* Jerarquia de Truco: a mayor valor devuelto, mas fuerte la carta.
 * 14 = 1 de espada (matador) ... 1 = cualquier 4 (la mas baja). */
unsigned char truco_power(Card c);

/* Compara dos cartas jugadas en una baza. Devuelve:
 *  1 si a le gana a b, -1 si b le gana a a, 0 si es parda (empate). */
signed char truco_compare(Card a, Card b);

#endif
