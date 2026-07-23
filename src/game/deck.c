#include "deck.h"

unsigned char card_number(Card c)
{
    unsigned char rank = CARD_RANK(c);
    static const unsigned char table[10] = { 1, 2, 3, 4, 5, 6, 7, 10, 11, 12 };
    return table[rank];
}

/* PRNG chico (xorshift16), suficiente para mezclar un mazo de 40 y liviano
 * para el 6502. No hace falta que sea criptografico. */
static unsigned int rng_state;

static unsigned int rng_next(void)
{
    rng_state ^= (unsigned int)(rng_state << 7);
    rng_state ^= (unsigned int)(rng_state >> 9);
    rng_state ^= (unsigned int)(rng_state << 8);
    return rng_state;
}

void deck_init_shuffled(Card deck[DECK_SIZE], unsigned int seed)
{
    unsigned char i;
    unsigned char j;
    Card tmp;

    rng_state = seed ? seed : 1;

    for (i = 0; i < DECK_SIZE; ++i) {
        deck[i] = i;
    }

    /* Fisher-Yates */
    for (i = DECK_SIZE - 1; i > 0; --i) {
        j = (unsigned char)(rng_next() % (i + 1));
        tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
}
