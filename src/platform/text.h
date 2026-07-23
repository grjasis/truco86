#ifndef TRUCO86_TEXT_H
#define TRUCO86_TEXT_H

/* Texto simple en el nametable, usando la fuente de letras generada por
 * tools/make_chr.py (ver docs/GRAFICOS.md). Solo soporta el subconjunto de
 * mayusculas usado por los menus y textos del juego (A,C,D,E,F,G,I,L,N,O,
 * P,Q,R,S,T,U,V) y el espacio ' '; cualquier otro caracter se dibuja en
 * blanco.
 *
 * Igual que con los tiles de carta (ver ppu_draw.h): si el render ya esta
 * prendido, hay que llamarlas justo despues de wait_vblank() y seguirlas
 * con ppu_finish_vram_update(). */

/* Tile de una letra soportada ('A'..'Z', ' '). TILE_BLANK si no esta
 * soportada. */
unsigned char letter_tile(char c);

/* Dibuja s (terminado en NUL) empezando en (col,row), un tile por caracter. */
void draw_text(unsigned char col, unsigned char row, const char *s);

/* Borra 'len' tiles de texto empezando en (col,row). */
void clear_text(unsigned char col, unsigned char row, unsigned char len);

/* Tile de un digito 0..9 (fondo blanco, como los numeros de carta). Fuera
 * de rango devuelve TILE_BLANK. */
unsigned char digit_tile(unsigned char d);

/* Dibuja 'value' (0..99) en 2 tiles empezando en (col,row); si value < 10
 * el tile de las decenas queda en blanco (sin cero a la izquierda). */
void draw_number(unsigned char col, unsigned char row, unsigned char value);

#endif
