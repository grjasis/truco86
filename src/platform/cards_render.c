#include "cards_render.h"
#include "ppu_draw.h"

/* Numero de una carta en 2 tiles (hi/lo), reusando los mismos tiles de
 * digito grande que el 1..7 suelto y el marcador (TILE_RANK_1..7,
 * TILE_DIGIT_0): asi el "10"/"11"/"12" se ve con la MISMA fuente grande
 * que el resto en vez de una fuente chica de 3x5 apretada en un solo tile
 * (eso era lo que se veia "chico y raro"). Para 1..7, hi queda en blanco
 * (numero de un solo digito, alineado a la derecha). */
static void rank_tiles_for(Card c, unsigned char *hi, unsigned char *lo)
{
    unsigned char number = card_number(c);

    if (number <= 7) {
        *hi = TILE_WHITE; /* blanco solido, no TILE_BLANK: esta sobre la carta blanca */
        *lo = (unsigned char)(TILE_RANK_1 + (number - 1));
        return;
    }

    *hi = TILE_RANK_1; /* el "1" de "10"/"11"/"12" */
    switch (number) {
        case 10: *lo = TILE_DIGIT_0; break;
        case 11: *lo = TILE_RANK_1; break;
        default:  *lo = (unsigned char)(TILE_RANK_1 + 1); break; /* 12 -> "2" */
    }
}

unsigned char suit_tile_for(Card c)
{
    /* CARD_SUIT: 0=espada,1=basto,2=oro,3=copa, mismo orden que los tiles
       TILE_SUIT_ESPADA..TILE_SUIT_COPA generados por tools/make_chr.py.
       Solo se usa para el adorno chico de la pantalla de titulo. */
    return (unsigned char)(TILE_SUIT_ESPADA + CARD_SUIT(c));
}

/* Palo grande (16x16 = 4 tiles) de una carta: TL/TR/BL/BR consecutivos por
   palo en tools/make_chr.py, ver TILE_SUIT_BIG_BASE. */
static void suit_tiles_for(Card c, unsigned char *tl, unsigned char *tr, unsigned char *bl, unsigned char *br)
{
    unsigned char base = (unsigned char)(TILE_SUIT_BIG_BASE + (unsigned char)(CARD_SUIT(c) * 4));
    *tl = base;
    *tr = (unsigned char)(base + 1);
    *bl = (unsigned char)(base + 2);
    *br = (unsigned char)(base + 3);
}

/* Paleta (ver PALETTE_* en cards_render.h y palette[] en main.c) para
 * pintar el palo grande de una carta de un color distinto por palo. */
static unsigned char palette_for_suit(Card c)
{
    switch (CARD_SUIT(c)) {
        case 0: return PALETTE_ESPADA;
        case 1: return PALETTE_BASTO;
        case 2: return PALETTE_ORO;
        default: return PALETTE_DEFAULT; /* copa: ya tenia tinta roja */
    }
}

/* Las cartas se dibujan verticales (numero arriba, palo abajo, mismas 2
 * columnas) para que la silueta se parezca a un naipe de verdad (mas alta
 * que ancha) en vez de un rectangulo horizontal. draw_card_border() agrega
 * el marco alrededor: col-1/col+2 a los costados, row-1 arriba y row+3
 * abajo. */
void draw_card_face(unsigned char col, unsigned char row, Card c)
{
    unsigned char hi, lo, tl, tr, bl, br;
    unsigned char col2 = (unsigned char)(col + 1);
    unsigned char pal = palette_for_suit(c);

    rank_tiles_for(c, &hi, &lo);
    suit_tiles_for(c, &tl, &tr, &bl, &br);

    ppu_set_tile(col, row, hi);
    ppu_set_tile(col2, row, lo);
    ppu_set_tile(col, (unsigned char)(row + 1), tl);
    ppu_set_tile(col2, (unsigned char)(row + 1), tr);
    ppu_set_tile(col, (unsigned char)(row + 2), bl);
    ppu_set_tile(col2, (unsigned char)(row + 2), br);

    /* Pinta el bloque del palo del color que le corresponde (ver
       palette_for_suit()). La tabla de atributos solo elige paleta de a
       bloques de 2x2 tiles, asi que se apunta a las 2 filas del palo (no
       a la del numero): segun como caigan esas filas en esa grilla de 2x2,
       puede llegar a pintar tambien el numero y/o el borde de abajo de la
       carta (parte del mismo bloque) — se acepta ese "sangrado" en vez de
       rehacer todo el layout de filas para que quede perfecto. */
    ppu_set_palette_block(col, (unsigned char)(row + 1), pal);
    ppu_set_palette_block(col, (unsigned char)(row + 2), pal);
}

void draw_card_back(unsigned char col, unsigned char row)
{
    unsigned char col2 = (unsigned char)(col + 1);
    ppu_set_tile(col, row, TILE_CARD_BACK);
    ppu_set_tile(col2, row, TILE_CARD_BACK);
    ppu_set_tile(col, (unsigned char)(row + 1), TILE_CARD_BACK);
    ppu_set_tile(col2, (unsigned char)(row + 1), TILE_CARD_BACK);
}

void clear_card(unsigned char col, unsigned char row)
{
    unsigned char col2 = (unsigned char)(col + 1);
    unsigned char r;
    for (r = 0; r < 3; ++r) {
        ppu_set_tile(col, (unsigned char)(row + r), TILE_BLANK);
        ppu_set_tile(col2, (unsigned char)(row + r), TILE_BLANK);
    }
}

void clear_card_back(unsigned char col, unsigned char row)
{
    unsigned char col2 = (unsigned char)(col + 1);
    ppu_set_tile(col, row, TILE_BLANK);
    ppu_set_tile(col2, row, TILE_BLANK);
    ppu_set_tile(col, (unsigned char)(row + 1), TILE_BLANK);
    ppu_set_tile(col2, (unsigned char)(row + 1), TILE_BLANK);
}

void draw_card_border(unsigned char col, unsigned char row)
{
    unsigned char left = (unsigned char)(col - 1);
    unsigned char inner_right = (unsigned char)(col + 1);
    unsigned char right = (unsigned char)(col + 2);
    unsigned char top = (unsigned char)(row - 1);
    unsigned char mid1 = (unsigned char)(row + 1);
    unsigned char mid2 = (unsigned char)(row + 2);
    unsigned char bottom = (unsigned char)(row + 3);

    ppu_set_tile(left, top, TILE_BORDER_TL);
    ppu_set_tile(col, top, TILE_BORDER_TOP);
    ppu_set_tile(inner_right, top, TILE_BORDER_TOP);
    ppu_set_tile(right, top, TILE_BORDER_TR);

    ppu_set_tile(left, row, TILE_BORDER_LEFT);
    ppu_set_tile(right, row, TILE_BORDER_RIGHT);
    ppu_set_tile(left, mid1, TILE_BORDER_LEFT);
    ppu_set_tile(right, mid1, TILE_BORDER_RIGHT);
    ppu_set_tile(left, mid2, TILE_BORDER_LEFT);
    ppu_set_tile(right, mid2, TILE_BORDER_RIGHT);

    ppu_set_tile(left, bottom, TILE_BORDER_BL);
    ppu_set_tile(col, bottom, TILE_BORDER_BOTTOM);
    ppu_set_tile(inner_right, bottom, TILE_BORDER_BOTTOM);
    ppu_set_tile(right, bottom, TILE_BORDER_BR);
}

void clear_card_border(unsigned char col, unsigned char row)
{
    unsigned char left = (unsigned char)(col - 1);
    unsigned char inner_right = (unsigned char)(col + 1);
    unsigned char right = (unsigned char)(col + 2);
    unsigned char top = (unsigned char)(row - 1);
    unsigned char mid1 = (unsigned char)(row + 1);
    unsigned char mid2 = (unsigned char)(row + 2);
    unsigned char bottom = (unsigned char)(row + 3);

    ppu_set_tile(left, top, TILE_BLANK);
    ppu_set_tile(col, top, TILE_BLANK);
    ppu_set_tile(inner_right, top, TILE_BLANK);
    ppu_set_tile(right, top, TILE_BLANK);

    ppu_set_tile(left, row, TILE_BLANK);
    ppu_set_tile(right, row, TILE_BLANK);
    ppu_set_tile(left, mid1, TILE_BLANK);
    ppu_set_tile(right, mid1, TILE_BLANK);
    ppu_set_tile(left, mid2, TILE_BLANK);
    ppu_set_tile(right, mid2, TILE_BLANK);

    ppu_set_tile(left, bottom, TILE_BLANK);
    ppu_set_tile(col, bottom, TILE_BLANK);
    ppu_set_tile(inner_right, bottom, TILE_BLANK);
    ppu_set_tile(right, bottom, TILE_BLANK);
}
