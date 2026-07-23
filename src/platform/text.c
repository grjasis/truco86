#include "text.h"
#include "cards_render.h"
#include "ppu_draw.h"

unsigned char letter_tile(char c)
{
    switch (c) {
        case 'A': return TILE_LETTER_A;
        case 'C': return TILE_LETTER_C;
        case 'D': return TILE_LETTER_D;
        case 'E': return TILE_LETTER_E;
        case 'F': return TILE_LETTER_F;
        case 'I': return TILE_LETTER_I;
        case 'L': return TILE_LETTER_L;
        case 'N': return TILE_LETTER_N;
        case 'O': return TILE_LETTER_O;
        case 'P': return TILE_LETTER_P;
        case 'Q': return TILE_LETTER_Q;
        case 'R': return TILE_LETTER_R;
        case 'S': return TILE_LETTER_S;
        case 'T': return TILE_LETTER_T;
        case 'U': return TILE_LETTER_U;
        case 'V': return TILE_LETTER_V;
        case 'G': return TILE_LETTER_G;
        case '.': return TILE_LETTER_DOT;
        default:  return TILE_BLANK; /* espacio u otro caracter no soportado */
    }
}

void draw_text(unsigned char col, unsigned char row, const char *s)
{
    unsigned char c = col;
    while (*s != '\0') {
        ppu_set_tile(c, row, letter_tile(*s));
        ++s;
        ++c;
    }
}

void clear_text(unsigned char col, unsigned char row, unsigned char len)
{
    unsigned char i;
    for (i = 0; i < len; ++i) {
        ppu_set_tile((unsigned char)(col + i), row, TILE_BLANK);
    }
}

unsigned char digit_tile(unsigned char d)
{
    if (d >= 1 && d <= 7) return (unsigned char)(TILE_RANK_1 + (d - 1));
    if (d == 0) return TILE_DIGIT_0;
    if (d == 8) return TILE_DIGIT_8;
    if (d == 9) return TILE_DIGIT_9;
    return TILE_BLANK;
}

void draw_number(unsigned char col, unsigned char row, unsigned char value)
{
    unsigned char tens = (unsigned char)(value / 10);
    unsigned char ones = (unsigned char)(value % 10);
    ppu_set_tile(col, row, tens ? digit_tile(tens) : TILE_BLANK);
    ppu_set_tile((unsigned char)(col + 1), row, digit_tile(ones));
}
