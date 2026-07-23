#include "card_rank.h"

/* Indexado por carta (suit*10+rank), ver docs/REGLAS.md para la tabla
 * completa de jerarquia con nombres. */
static const unsigned char power_table[DECK_SIZE] = {
    /* espada: As,2,3,4,5,6,7,10,11,12 */
    14, 9, 10, 1, 2, 3, 12, 5, 6, 7,
    /* basto */
    13, 9, 10, 1, 2, 3, 4, 5, 6, 7,
    /* oro */
     8, 9, 10, 1, 2, 3, 11, 5, 6, 7,
    /* copa */
     8, 9, 10, 1, 2, 3, 4, 5, 6, 7
};

unsigned char truco_power(Card c)
{
    return power_table[c];
}

signed char truco_compare(Card a, Card b)
{
    unsigned char pa = truco_power(a);
    unsigned char pb = truco_power(b);
    if (pa > pb) return 1;
    if (pb > pa) return -1;
    return 0;
}
