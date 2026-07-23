#ifndef TRUCO86_MATCH_H
#define TRUCO86_MATCH_H

#include "deck.h"

#define TRICK_TIE    0
#define TRICK_PLAYER 1
#define TRICK_CPU    2

/* Resultado de una baza: compara la carta del jugador contra la de la CPU
 * usando la jerarquia de truco (ver card_rank.h). */
unsigned char trick_result(Card player_card, Card cpu_card);

/* Dados los resultados de las 3 bazas de una mano (TRICK_TIE si alguna no
 * se llego a jugar), devuelve quien gana la mano: TRICK_PLAYER, TRICK_CPU,
 * o TRICK_TIE si las 3 bazas quedan pardas. Reglas de desempate (ver
 * docs/REGLAS.md):
 *  - si la 1ra baza no es parda: gana quien gane 2 de las 3 (si la 2da es
 *    parda o repite ganador, no hace falta la 3ra; si la 2da la gana el
 *    otro, decide la 3ra, y si esa tambien es parda, gana el de la 1ra).
 *  - si la 1ra baza es parda: gana quien gane la 2da (o si tambien es
 *    parda, quien gane la 3ra; si las 3 son pardas, la mano es parda). */
unsigned char hand_winner(const unsigned char results[3]);

/* Dadas las primeras 2 bazas ya jugadas (results[0], results[1]; results[2]
 * se ignora), dice si la mano ya esta decidida y por lo tanto no hace
 * falta jugar la 3ra baza: alguien ya se llevo 2 de las 2 jugadas (mismo
 * ganador en ambas), o la 1ra fue parda y la 2da no (decide sola, ver
 * hand_winner()). Si la 1ra no fue parda pero la 2da la gano el otro lado,
 * o si las 2 primeras fueron pardas, hace falta la 3ra. */
unsigned char hand_decided_after_two(const unsigned char results[2]);

#endif
