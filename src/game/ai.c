#include "ai.h"
#include "card_rank.h"

static unsigned char find_weakest_unplayed(const Card hand[3], const unsigned char played[3])
{
    unsigned char weakest_idx = 0xFF;
    unsigned char weakest_power = 0xFF;
    unsigned char i;

    for (i = 0; i < 3; ++i) {
        unsigned char p;
        if (played[i]) continue;
        p = truco_power(hand[i]);
        if (p < weakest_power) {
            weakest_power = p;
            weakest_idx = i;
        }
    }
    return weakest_idx;
}

unsigned char ai_choose_card(const Card hand[3], const unsigned char played[3], Card opponent_card)
{
    unsigned char best_win_idx = 0xFF;
    unsigned char best_win_power = 0xFF;
    unsigned char i;

    for (i = 0; i < 3; ++i) {
        unsigned char p;
        if (played[i]) continue;

        p = truco_power(hand[i]);
        if (truco_compare(hand[i], opponent_card) > 0 && p < best_win_power) {
            best_win_power = p;
            best_win_idx = i;
        }
    }

    return (best_win_idx != 0xFF) ? best_win_idx : find_weakest_unplayed(hand, played);
}

unsigned char ai_choose_lead_card(const Card hand[3], const unsigned char played[3])
{
    return find_weakest_unplayed(hand, played);
}

unsigned char ai_envido_accept(unsigned char own_score, unsigned char value, unsigned char entropy)
{
    unsigned char threshold = (unsigned char)(18 + (entropy % 5)); /* 18..22 */
    (void)value; /* el umbral no depende de cuanto se apuesta, solo de la propia mano */
    return own_score >= threshold;
}

unsigned char ai_envido_escalate(unsigned char own_score, unsigned char entropy)
{
    unsigned char threshold = (unsigned char)(25 + (entropy % 5)); /* 25..29 */
    return own_score >= threshold;
}

unsigned char ai_truco_accept(unsigned char best_power, unsigned char entropy)
{
    unsigned char threshold = (unsigned char)(7 + (entropy % 3)); /* 7..9 */
    return best_power >= threshold;
}

unsigned char ai_truco_escalate(unsigned char best_power, unsigned char entropy)
{
    unsigned char threshold = (unsigned char)(11 + (entropy % 3)); /* 11..13 */
    return best_power >= threshold;
}
