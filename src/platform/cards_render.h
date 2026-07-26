#ifndef TRUCO86_CARDS_RENDER_H
#define TRUCO86_CARDS_RENDER_H

#include "../game/deck.h"

/* IDs de tile, deben coincidir con el orden generado por tools/make_chr.py
 * (ver docs/GRAFICOS.md para la tabla completa). */
#define TILE_BLANK    0
#define TILE_RANK_1   1
#define TILE_RANK_7   7   /* TILE_RANK_1 .. TILE_RANK_7 son consecutivos */
#define TILE_RANK_10  8
#define TILE_RANK_11  9
#define TILE_RANK_12  10
#define TILE_SUIT_ESPADA 11
#define TILE_SUIT_BASTO  12
#define TILE_SUIT_ORO    13
#define TILE_SUIT_COPA   14
#define TILE_CARD_BACK   15
#define TILE_CURSOR      16
#define TILE_LETTER_A    17
#define TILE_LETTER_D    18
#define TILE_LETTER_E    19
#define TILE_LETTER_F    20
#define TILE_LETTER_I    21
#define TILE_LETTER_L    22
#define TILE_LETTER_N    23
#define TILE_LETTER_O    24
#define TILE_LETTER_P    25
#define TILE_LETTER_Q    26
#define TILE_LETTER_R    27
#define TILE_LETTER_S    28
#define TILE_LETTER_T    29
#define TILE_LETTER_U    30
#define TILE_LETTER_V    31
#define TILE_LETTER_C    32
#define TILE_DIGIT_0     33
#define TILE_DIGIT_8     34
#define TILE_DIGIT_9     35
#define TILE_LETTER_G    36
#define TILE_BORDER_TOP     37
#define TILE_BORDER_BOTTOM  38
#define TILE_BORDER_LEFT    39
#define TILE_BORDER_RIGHT   40
#define TILE_BORDER_TL      41
#define TILE_BORDER_TR      42
#define TILE_BORDER_BL      43
#define TILE_BORDER_BR      44

/* Palos grandes (16x16 = 4 tiles de 8x8), para el numero/palo de cada
 * carta en mesa (ver draw_card_face()). TL/TR/BL/BR de cada palo son
 * consecutivos, en el mismo orden espada/basto/oro/copa que arriba: espada
 * empieza en TILE_SUIT_BIG_BASE, basto en +4, oro en +8, copa en +12. */
#define TILE_SUIT_BIG_BASE  45
#define TILE_WHITE          61 /* blanco SOLIDO (a diferencia de TILE_BLANK, que es transparente/color de fondo del pano) */
#define TILE_LETTER_DOT     62 /* "." para "CODE.AR" en la pantalla de titulo */

/* Digitos de la INTERFAZ (blancos con sombra, fondo transparente): marcador,
 * menu del titulo, "VALE n". TILE_HUD_DIGIT_BASE + d, d = 0..9. Distintos de
 * los TILE_RANK_x y TILE_DIGIT_x de arriba, que tienen fondo blanco porque van impresos sobre
 * la cara de una carta. */
#define TILE_HUD_DIGIT_BASE 63

/* Dorso de carta (16x16 = 4 tiles: TL/TR/BL/BR consecutivos), para la mano
 * tapada de la CPU. Ver draw_card_back(). */
#define TILE_BACK_BASE      73

#define TILE_LETTER_M       77 /* "MANO" */

/* Marca del resultado de una baza, al costado de su columna (ver
 * draw_trick_mark() en main.c). */
#define TILE_MARK_PLAYER    78 /* triangulo hacia abajo: la gano el jugador */
#define TILE_MARK_CPU       79 /* triangulo hacia arriba: la gano la CPU */
#define TILE_MARK_TIE       80 /* "=": parda */

/* Puntero ancho (2 tiles) que marca la carta elegida, y flecha del cursor de
 * los menus. TILE_CURSOR (el de 8x8) queda solo por compatibilidad. */
#define TILE_CURSOR_WIDE_L  81
#define TILE_CURSOR_WIDE_R  82
#define TILE_ARROW_RIGHT    83

/* Marco fino BLANCO sobre el paño (distinto de TILE_BORDER_*, que es el marco
 * negro sobre blanco de una carta): encuadra la pantalla de titulo y la de
 * resultado. Ver draw_frame() en text.h. */
#define TILE_UI_H           84
#define TILE_UI_VL          85
#define TILE_UI_VR          86
#define TILE_UI_TL          87
#define TILE_UI_TR          88
#define TILE_UI_BL          89
#define TILE_UI_BR          90

/* Paletas de fondo (ver main.c, palette[]) para pintar cada palo de un
 * color distinto (espada=azul, basto=verde, oro=amarillo, copa=rojo, con
 * la paleta 0 que ya se usaba antes de agregar esto): asignadas por
 * draw_card_face() via ppu_set_palette_block() (ver ppu_draw.h). */
#define PALETTE_DEFAULT 0 /* tambien la de copa: ya usaba tinta roja */
#define PALETTE_ESPADA  1
#define PALETTE_BASTO   2
#define PALETTE_ORO     3

/* Tile del palo CHICO (espada/basto/oro/copa) para una carta del motor de
 * reglas: ya no se usa para las cartas en mesa (ver draw_card_face(), que
 * usa los palos grandes de arriba), solo para el adorno de la pantalla de
 * titulo (ver TITLE_SUITS_* en main.c). */
unsigned char suit_tile_for(Card c);

/* Dibuja una carta boca arriba en el nametable: numero (2 tiles de ancho,
 * arriba) y palo (2x2 tiles, abajo) empezando en (col,row) — la carta
 * completa (con draw_card_border()) ocupa col-1..col+2 x row-1..row+3, asi
 * que esas celdas de alrededor tienen que estar libres. Debe llamarse con
 * el render apagado (ver ppu_draw.h). */
void draw_card_face(unsigned char col, unsigned char row, Card c);

/* Dibuja una carta boca abajo (dorso) de 2x2 tiles en (col,row): mas chica
 * que draw_card_face() a proposito (no hace falta mostrar numero/palo en
 * una carta tapada, asi que no ocupa el mismo alto). */
void draw_card_back(unsigned char col, unsigned char row);

/* Borra los 2x3 tiles de una carta boca arriba dibujada con
 * draw_card_face() (la deja en blanco/transparente). */
void clear_card(unsigned char col, unsigned char row);

/* Borra los 2x2 tiles de un dorso dibujado con draw_card_back(). */
void clear_card_back(unsigned char col, unsigned char row);

/* Dibuja un marco de 1px alrededor de una carta ya dibujada en (col,row)
 * con draw_card_face(): ocupa col-1/col+2 a los costados y row-1/row+3
 * arriba y abajo. */
void draw_card_border(unsigned char col, unsigned char row);

/* Borra el marco dibujado por draw_card_border() (no borra el numero/palo,
 * usar clear_card() para eso). */
void clear_card_border(unsigned char col, unsigned char row);

#endif
