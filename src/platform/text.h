#ifndef TRUCO86_TEXT_H
#define TRUCO86_TEXT_H

/* Texto y marcos de la interfaz en el nametable, usando la fuente de letras
 * generada por tools/make_chr.py (ver docs/GRAFICOS.md). Las letras son
 * BLANCAS con sombra negra sobre fondo transparente: se leen sobre el verde
 * del paño mucho mejor que la tinta negra que se usaba antes. Solo soporta el
 * subconjunto de mayusculas usado por los menus y textos del juego (A,C,D,E,
 * F,G,I,L,M,N,O,P,Q,R,S,T,U,V), el punto '.' (para "CODE.AR") y el espacio
 * ' '; cualquier otro caracter se dibuja en blanco.
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

/* Tile de un digito 0..9 de la INTERFAZ (blanco con sombra, fondo
 * transparente, igual que las letras). Fuera de rango devuelve TILE_BLANK.
 * Los numeros impresos en la cara de una carta NO usan esto (van sobre fondo
 * blanco, ver rank_tiles_for() en cards_render.c). */
unsigned char digit_tile(unsigned char d);

/* Dibuja 'value' (0..99) en 2 tiles empezando en (col,row); si value < 10
 * el tile de las decenas queda en blanco (sin cero a la izquierda). */
void draw_number(unsigned char col, unsigned char row, unsigned char value);

/* Dibuja un marco fino blanco de w x h tiles con la esquina superior
 * izquierda en (col,row). Solo dibuja el contorno, no borra el interior. */
void draw_frame(unsigned char col, unsigned char row, unsigned char w, unsigned char h);

/* Dibuja una linea horizontal fina de 'len' tiles desde (col,row) (separador
 * entre el marcador y la mesa). */
void draw_rule(unsigned char col, unsigned char row, unsigned char len);

#endif
