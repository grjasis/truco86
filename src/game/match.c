#include "match.h"
#include "card_rank.h"

unsigned char trick_result(Card player_card, Card cpu_card)
{
    signed char cmp = truco_compare(player_card, cpu_card);
    if (cmp > 0) return TRICK_PLAYER;
    if (cmp < 0) return TRICK_CPU;
    return TRICK_TIE;
}

unsigned char hand_winner(const unsigned char results[3])
{
    if (results[0] != TRICK_TIE) {
        if (results[1] == results[0]) return results[0];
        if (results[1] == TRICK_TIE) return results[0];
        /* results[1] es del otro lado: decide la 3ra baza */
        if (results[2] != TRICK_TIE) return results[2];
        return results[0];
    }

    /* 1ra baza parda */
    if (results[1] != TRICK_TIE) return results[1];
    if (results[2] != TRICK_TIE) return results[2];
    return TRICK_TIE;
}

unsigned char hand_decided_after_two(const unsigned char results[2])
{
    if (results[0] != TRICK_TIE) {
        return (unsigned char)(results[1] == TRICK_TIE || results[1] == results[0]);
    }
    return (unsigned char)(results[1] != TRICK_TIE);
}
