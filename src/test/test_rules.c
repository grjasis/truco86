/* Tests de host (gcc/clang) para el motor de reglas puro de Truco86.
 * No depende de hardware NES: valida deck, jerarquia de truco, envido y flor
 * contra casos documentados en docs/REGLAS.md. Correr con `make test`. */

#include <stdio.h>
#include <string.h>
#include "../game/deck.h"
#include "../game/card_rank.h"
#include "../game/envido.h"
#include "../game/flor.h"
#include "../game/match.h"
#include "../game/canto.h"
#include "../game/ai.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
        if (!(cond)) { \
            printf("FALLO: %s (linea %d)\n", msg, __LINE__); \
            failures++; \
        } else { \
            printf("ok: %s\n", msg); \
        } \
    } while (0)

/* Construye una carta a partir de palo (0..3) y numero real (1..7,10,11,12) */
static Card mk(unsigned char suit, unsigned char number)
{
    static const unsigned char rank_for_number[13] = {
        255, 0, 1, 2, 3, 4, 5, 6, 255, 255, 7, 8, 9
    };
    return (Card)(suit * 10 + rank_for_number[number]);
}

static void test_deck(void)
{
    Card deck[DECK_SIZE];
    unsigned char seen[DECK_SIZE];
    unsigned char i;
    unsigned char all_present = 1;

    memset(seen, 0, sizeof(seen));
    deck_init_shuffled(deck, 12345);

    for (i = 0; i < DECK_SIZE; ++i) {
        seen[deck[i]]++;
    }
    for (i = 0; i < DECK_SIZE; ++i) {
        if (seen[i] != 1) all_present = 0;
    }
    CHECK(all_present, "deck mezclado contiene las 40 cartas sin repetidos");

    CHECK(card_number(mk(SUIT_ESPADA, 1)) == 1, "card_number As");
    CHECK(card_number(mk(SUIT_ESPADA, 12)) == 12, "card_number Doce");
}

static void test_truco_rank(void)
{
    CHECK(truco_compare(mk(SUIT_ESPADA, 1), mk(SUIT_BASTO, 1)) == 1,
          "1 de espada le gana al 1 de basto");
    CHECK(truco_compare(mk(SUIT_BASTO, 1), mk(SUIT_ESPADA, 7)) == 1,
          "1 de basto le gana al 7 de espada");
    CHECK(truco_compare(mk(SUIT_ESPADA, 7), mk(SUIT_ORO, 7)) == 1,
          "7 de espada le gana al 7 de oro");
    CHECK(truco_compare(mk(SUIT_ORO, 7), mk(SUIT_COPA, 3)) == 1,
          "7 de oro le gana a cualquier 3");
    CHECK(truco_compare(mk(SUIT_ESPADA, 3), mk(SUIT_COPA, 3)) == 0,
          "3 contra 3 de otro palo es parda");
    CHECK(truco_compare(mk(SUIT_ORO, 1), mk(SUIT_COPA, 1)) == 0,
          "ancho falso de oro contra ancho falso de copa es parda");
    CHECK(truco_compare(mk(SUIT_BASTO, 7), mk(SUIT_COPA, 7)) == 0,
          "siete falso de basto contra siete falso de copa es parda");
    CHECK(truco_compare(mk(SUIT_ESPADA, 4), mk(SUIT_BASTO, 5)) == -1,
          "el 4 es la carta mas baja del juego");
    CHECK(truco_compare(mk(SUIT_ORO, 12), mk(SUIT_ESPADA, 10)) == 1,
          "el 12 le gana al 10");
}

static void test_envido(void)
{
    Card hand1[3] = { mk(SUIT_ESPADA, 7), mk(SUIT_ESPADA, 6), mk(SUIT_BASTO, 1) };
    Card hand2[3] = { mk(SUIT_ORO, 12), mk(SUIT_ORO, 11), mk(SUIT_COPA, 10) };
    Card hand3[3] = { mk(SUIT_ESPADA, 4), mk(SUIT_BASTO, 6), mk(SUIT_ORO, 2) };

    CHECK(envido_score(hand1) == 33, "7 y 6 de espada = 33 de envido");
    CHECK(envido_score(hand2) == 20, "figuras del mismo palo = 20");
    CHECK(envido_score(hand3) == 6, "sin pareja de palo, vale la carta suelta mas alta");
}

static void test_flor(void)
{
    Card flor_hand[3] = { mk(SUIT_COPA, 4), mk(SUIT_COPA, 5), mk(SUIT_COPA, 12) };
    Card no_flor[3] = { mk(SUIT_COPA, 4), mk(SUIT_ORO, 5), mk(SUIT_COPA, 12) };

    CHECK(flor_has(flor_hand) == 1, "3 copas es flor");
    CHECK(flor_score(flor_hand) == 29, "flor de 4,5,12 de copa = 29");
    CHECK(flor_has(no_flor) == 0, "palos mezclados no es flor");
}

static void test_match(void)
{
    unsigned char r1[3] = { TRICK_PLAYER, TRICK_CPU, TRICK_PLAYER };
    unsigned char r2[3] = { TRICK_PLAYER, TRICK_CPU, TRICK_CPU };
    unsigned char r3[3] = { TRICK_PLAYER, TRICK_CPU, TRICK_TIE };
    unsigned char r4[3] = { TRICK_PLAYER, TRICK_PLAYER, TRICK_TIE };
    unsigned char r5[3] = { TRICK_PLAYER, TRICK_TIE, TRICK_CPU };
    unsigned char r6[3] = { TRICK_TIE, TRICK_PLAYER, TRICK_TIE };
    unsigned char r7[3] = { TRICK_TIE, TRICK_TIE, TRICK_PLAYER };
    unsigned char r8[3] = { TRICK_TIE, TRICK_TIE, TRICK_TIE };

    CHECK(trick_result(mk(SUIT_ESPADA, 1), mk(SUIT_BASTO, 1)) == TRICK_PLAYER,
          "1 de espada le gana la baza al 1 de basto");
    CHECK(trick_result(mk(SUIT_ESPADA, 3), mk(SUIT_COPA, 3)) == TRICK_TIE,
          "3 contra 3 es baza parda");
    CHECK(trick_result(mk(SUIT_ESPADA, 4), mk(SUIT_BASTO, 5)) == TRICK_CPU,
          "4 pierde la baza contra el 5");

    CHECK(hand_winner(r1) == TRICK_PLAYER, "jugador-cpu-jugador: decide la 3ra, gana jugador");
    CHECK(hand_winner(r2) == TRICK_CPU, "jugador-cpu-cpu: decide la 3ra, gana cpu");
    CHECK(hand_winner(r3) == TRICK_PLAYER, "jugador-cpu-parda: 3ra parda, gana el de la 1ra");
    CHECK(hand_winner(r4) == TRICK_PLAYER, "jugador-jugador-*: gana en la 2da, no hace falta la 3ra");
    CHECK(hand_winner(r5) == TRICK_PLAYER, "jugador-parda-*: la 2da parda no cambia al ganador de la 1ra");
    CHECK(hand_winner(r6) == TRICK_PLAYER, "parda-jugador-*: la 2da decide");
    CHECK(hand_winner(r7) == TRICK_PLAYER, "parda-parda-jugador: decide la 3ra");
    CHECK(hand_winner(r8) == TRICK_TIE, "parda-parda-parda: la mano entera es parda");

    CHECK(hand_decided_after_two(r2) == 0, "jugador-cpu: distinto ganador en cada una, hace falta la 3ra");
    CHECK(hand_decided_after_two(r4) == 1, "jugador-jugador: ya gano 2 de 2, no hace falta la 3ra");
    CHECK(hand_decided_after_two(r5) == 1, "jugador-parda: la 1ra ya decide, no hace falta la 3ra");
    CHECK(hand_decided_after_two(r6) == 1, "parda-jugador: la 2da ya decide, no hace falta la 3ra");
    CHECK(hand_decided_after_two(r7) == 0, "parda-parda: hace falta la 3ra");
}

static void test_canto_envido(void)
{
    EnvidoChain c0 = { 0, 0, 0 };
    EnvidoChain c1 = { 1, 0, 0 }; /* envido */
    EnvidoChain c2 = { 2, 0, 0 }; /* envido, envido */
    EnvidoChain cr = { 0, 1, 0 }; /* real envido solo */
    EnvidoChain c1r = { 1, 1, 0 }; /* envido + real envido */
    EnvidoChain c2r = { 2, 1, 0 }; /* envido, envido + real envido */
    EnvidoChain cf = { 0, 0, 1 }; /* falta envido solo */

    CHECK(envido_quiero_value(&c1, 0) == 2, "envido querido vale 2");
    CHECK(envido_no_quiero_value(&c0) == 1, "envido no querido (primer canto) vale 1");

    CHECK(envido_quiero_value(&c2, 0) == 4, "envido-envido querido vale 4");
    CHECK(envido_no_quiero_value(&c1) == 2, "envido-envido no querido vale 2 (lo del envido anterior)");

    CHECK(envido_quiero_value(&cr, 0) == 3, "real envido solo querido vale 3");
    CHECK(envido_no_quiero_value(&c0) == 1, "real envido solo no querido vale 1");

    CHECK(envido_quiero_value(&c1r, 0) == 5, "envido + real envido querido vale 5");
    CHECK(envido_no_quiero_value(&c1) == 2, "envido + real envido no querido vale 2");

    CHECK(envido_quiero_value(&c2r, 0) == 7, "envido-envido + real envido querido vale 7");
    CHECK(envido_no_quiero_value(&c2) == 4, "envido-envido + real envido no querido vale 4");

    CHECK(envido_quiero_value(&cf, 12) == 12, "falta envido querido vale lo que le falta al que gana");
    CHECK(envido_no_quiero_value(&c0) == 1, "falta envido (primer canto) no querido vale 1");
    CHECK(envido_no_quiero_value(&c2) == 4, "falta envido despues de envido-envido no querido vale 4");

    CHECK(envido_can_sing_envido(&c0) == 1, "se puede cantar envido al principio");
    CHECK(envido_can_sing_envido(&c2) == 0, "no se puede cantar un tercer envido");
    CHECK(envido_can_sing_envido(&cr) == 0, "no se puede cantar envido despues de real envido");
    CHECK(envido_can_sing_real(&cr) == 0, "no se puede cantar real envido dos veces");
    CHECK(envido_can_sing_falta(&cf) == 0, "no se puede cantar nada despues de falta envido");
}

static void test_canto_flor(void)
{
    FlorChain f0 = { FLOR_STEP_NONE };
    FlorChain f1 = { FLOR_STEP_FLOR };
    FlorChain f2 = { FLOR_STEP_CONTRAFLOR };
    FlorChain f3 = { FLOR_STEP_CONTRAFLOR_RESTO };

    CHECK(flor_quiero_value(&f1, 0) == 3, "flor querida vale 3");
    CHECK(flor_no_quiero_value(&f0) == 1, "flor (primer canto) no querida vale 1");

    CHECK(flor_quiero_value(&f2, 0) == 6, "contraflor querida vale 6");
    CHECK(flor_no_quiero_value(&f1) == 3, "contraflor no querida vale 3 (lo de la flor)");

    CHECK(flor_quiero_value(&f3, 15) == 15, "contraflor al resto vale lo que falta para ganar");
    CHECK(flor_no_quiero_value(&f2) == 6, "contraflor al resto no querida vale 6 (lo de la contraflor)");

    CHECK(flor_can_sing_contraflor(&f1) == 1, "se puede escalar flor a contraflor");
    CHECK(flor_can_sing_contraflor(&f0) == 0, "no se puede cantar contraflor sin flor previa");
    CHECK(flor_can_sing_contraflor_resto(&f2) == 1, "se puede escalar contraflor a contraflor al resto");
}

static void test_canto_truco(void)
{
    TrucoChain t0 = { TRUCO_STEP_NONE };
    TrucoChain t1 = { TRUCO_STEP_TRUCO };
    TrucoChain t2 = { TRUCO_STEP_RETRUCO };
    TrucoChain t3 = { TRUCO_STEP_VALE_CUATRO };

    CHECK(truco_hand_value(&t0) == 1, "sin cantar nada la mano vale 1");
    CHECK(truco_quiero_value(TRUCO_STEP_TRUCO) == 2, "truco querido vale 2");
    CHECK(truco_no_quiero_value(&t0) == 1, "truco no querido vale 1 (lo que ya valia)");

    CHECK(truco_quiero_value(TRUCO_STEP_RETRUCO) == 3, "retruco querido vale 3");
    CHECK(truco_no_quiero_value(&t1) == 2, "retruco no querido vale 2 (lo del truco)");

    CHECK(truco_quiero_value(TRUCO_STEP_VALE_CUATRO) == 4, "vale cuatro querido vale 4");
    CHECK(truco_no_quiero_value(&t2) == 3, "vale cuatro no querido vale 3 (lo del retruco)");

    CHECK(truco_can_sing_truco(&t0) == 1, "se puede cantar truco al principio");
    CHECK(truco_can_sing_truco(&t1) == 0, "no se puede cantar truco dos veces");
    CHECK(truco_can_sing_retruco(&t1) == 1, "se puede escalar truco a retruco");
    CHECK(truco_can_sing_retruco(&t0) == 0, "no se puede cantar retruco sin truco previo");
    CHECK(truco_can_sing_vale_cuatro(&t2) == 1, "se puede escalar retruco a vale cuatro");
    CHECK(truco_can_sing_vale_cuatro(&t3) == 0, "no se puede escalar mas alla de vale cuatro");
}

static void test_ai(void)
{
    Card hand[3] = { mk(SUIT_ESPADA, 4), mk(SUIT_ESPADA, 1), mk(SUIT_ORO, 7) };
    unsigned char none_played[3] = { 0, 0, 0 };
    unsigned char matador_played[3] = { 0, 1, 0 };

    CHECK(ai_choose_card(hand, none_played, mk(SUIT_ESPADA, 3)) == 2,
          "con 3 en la mesa, gana con el 7 de oro (la ganadora mas floja), no con el matador");
    CHECK(ai_choose_card(hand, none_played, mk(SUIT_ESPADA, 1)) == 0,
          "contra el matador no se puede ganar: sacrifica el 4 (la mas floja)");
    CHECK(ai_choose_card(hand, matador_played, mk(SUIT_BASTO, 1)) == 0,
          "sin el matador en la mano, no le gana al 1 de basto: sacrifica la mas floja disponible");

    CHECK(ai_envido_accept(20, 2, 0) == 1, "con 20 de envido acepta (umbral base 18)");
    CHECK(ai_envido_accept(17, 2, 0) == 0, "con 17 de envido no acepta (umbral base 18)");
    CHECK(ai_envido_accept(21, 2, 4) == 0, "el entropy puede subir el umbral hasta 22");

    CHECK(ai_envido_escalate(29, 4) == 1, "con 29 de envido escala (umbral con entropy 4 = 29)");
    CHECK(ai_envido_escalate(24, 0) == 0, "con 24 de envido no escala (umbral base 25)");

    CHECK(ai_truco_accept(9, 2) == 1, "con poder 9 acepta truco (umbral con entropy 2 = 9)");
    CHECK(ai_truco_accept(6, 0) == 0, "con poder 6 no acepta truco (umbral base 7)");

    CHECK(ai_truco_escalate(13, 2) == 1, "con poder 13 escala (umbral con entropy 2 = 13)");
    CHECK(ai_truco_escalate(10, 0) == 0, "con poder 10 no escala (umbral base 11)");
}

int main(void)
{
    test_deck();
    test_truco_rank();
    test_envido();
    test_flor();
    test_match();
    test_canto_envido();
    test_canto_flor();
    test_canto_truco();
    test_ai();

    if (failures == 0) {
        printf("\nTODO OK (0 fallos)\n");
        return 0;
    }
    printf("\n%d fallo(s)\n", failures);
    return 1;
}
