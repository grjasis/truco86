#include "envido.h"

unsigned char envido_card_value(Card c)
{
    unsigned char rank = CARD_RANK(c);
    if (rank <= 6) return rank + 1; /* As..Siete -> 1..7 */
    return 0;                       /* 10, 11, 12 (figuras) */
}

unsigned char envido_score(const Card hand[3])
{
    unsigned char best = 0;
    unsigned char found_pair = 0;
    unsigned char i, j;

    for (i = 0; i < 3; ++i) {
        for (j = i + 1; j < 3; ++j) {
            if (CARD_SUIT(hand[i]) == CARD_SUIT(hand[j])) {
                unsigned char sum = (unsigned char)(20 + envido_card_value(hand[i]) + envido_card_value(hand[j]));
                if (sum > best) best = sum;
                found_pair = 1;
            }
        }
    }

    if (!found_pair) {
        for (i = 0; i < 3; ++i) {
            unsigned char v = envido_card_value(hand[i]);
            if (v > best) best = v;
        }
    }

    return best;
}
