#ifndef TRUCO86_DECK_H
#define TRUCO86_DECK_H

/* Mazo espanol de 40 cartas (sin 8, 9 ni comodines).
 * Una carta se representa como un byte 0..39:
 *   suit = card / 10   (0=espada, 1=basto, 2=oro, 3=copa)
 *   rank = card % 10   (0=As, 1=Dos, ... 6=Siete, 7=Diez(Sota), 8=Once(Caballo), 9=Doce(Rey))
 * rank 7,8,9 son las figuras (10,11,12) y valen 0 puntos de envido.
 */

typedef unsigned char Card;

#define SUIT_ESPADA 0
#define SUIT_BASTO  1
#define SUIT_ORO    2
#define SUIT_COPA   3

#define DECK_SIZE 40

#define CARD_SUIT(c) ((c) / 10)
#define CARD_RANK(c) ((c) % 10)  /* 0..9, ver arriba */

/* Numero "real" de la carta tal como se dice en la mesa (1..7,10,11,12) */
unsigned char card_number(Card c);

/* Llena deck[0..39] con las 40 cartas en orden y las mezcla con el PRNG
 * dado (semilla externa: en el NES se alimenta con ruido de hardware/estado
 * de juego; en tests de host se puede fijar para reproducibilidad). */
void deck_init_shuffled(Card deck[DECK_SIZE], unsigned int seed);

#endif
