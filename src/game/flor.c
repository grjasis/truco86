#include "flor.h"
#include "envido.h"

unsigned char flor_has(const Card hand[3])
{
    return (CARD_SUIT(hand[0]) == CARD_SUIT(hand[1])) &&
           (CARD_SUIT(hand[1]) == CARD_SUIT(hand[2]));
}

unsigned char flor_score(const Card hand[3])
{
    return (unsigned char)(20 + envido_card_value(hand[0]) +
                                 envido_card_value(hand[1]) +
                                 envido_card_value(hand[2]));
}
