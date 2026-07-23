#include <nes.h>
#include "ppu_draw.h"

/* Copia en RAM de la tabla de atributos (64 bytes, $23C0-$23FF): cada byte
   cubre un bloque de 4x4 tiles, con 2 bits de paleta por cada uno de sus 4
   cuadrantes de 2x2 tiles. Se mantiene sincronizada con la VRAM real para
   poder hacer leer-modificar-escribir sin tener que leer $2007 (que en el
   hardware real devuelve el valor del byte ANTERIOR por el buffer interno
   de lectura de la PPU, asi que leer VRAM directamente es mas delicado que
   escribirla). */
static unsigned char attr_shadow[64];

void ppu_clear_nametable0(void)
{
    unsigned int i;

    PPU.vram.address = 0x20;
    PPU.vram.address = 0x00;
    for (i = 0; i < 960; ++i) {
        PPU.vram.data = 0x00;
    }
    /* la tabla de atributos son los siguientes 64 bytes ($23C0-$23FF),
       ya quedamos posicionados justo despues del nametable */
    for (i = 0; i < 64; ++i) {
        PPU.vram.data = 0x00;
        attr_shadow[i] = 0x00;
    }
}

void ppu_set_palette_block(unsigned char col, unsigned char row, unsigned char palette)
{
    unsigned char cell_col = (unsigned char)(col / 4);
    unsigned char cell_row = (unsigned char)(row / 4);
    unsigned char qx = (unsigned char)((col / 2) & 0x01); /* 0=mitad izquierda, 1=derecha */
    unsigned char qy = (unsigned char)((row / 2) & 0x01); /* 0=mitad de arriba, 1=abajo */
    unsigned char shift = (unsigned char)(((qy << 1) | qx) << 1);
    unsigned char mask = (unsigned char)(0x03 << shift);
    unsigned char idx = (unsigned char)(cell_row * 8 + cell_col);
    unsigned int addr = 0x23C0 + idx;

    attr_shadow[idx] = (unsigned char)((attr_shadow[idx] & (unsigned char)~mask) | (unsigned char)((palette & 0x03) << shift));

    PPU.vram.address = (unsigned char)(addr >> 8);
    PPU.vram.address = (unsigned char)(addr & 0xFF);
    PPU.vram.data = attr_shadow[idx];
}

void ppu_load_palette(const unsigned char pal[32])
{
    unsigned char i;

    PPU.vram.address = 0x3F;
    PPU.vram.address = 0x00;
    for (i = 0; i < 32; ++i) {
        PPU.vram.data = pal[i];
    }
}

void ppu_set_tile(unsigned char col, unsigned char row, unsigned char tile)
{
    unsigned int addr = 0x2000 + ((unsigned int)row * 32) + col;
    PPU.vram.address = (unsigned char)(addr >> 8);
    PPU.vram.address = (unsigned char)(addr & 0xFF);
    PPU.vram.data = tile;
}

void ppu_clear_oam(void)
{
    unsigned int i;
    PPU.sprite.address = 0x00;
    for (i = 0; i < 256; ++i) {
        PPU.sprite.data = 0xFF;
    }
}

void ppu_reset_scroll(void)
{
    PPU.scroll = 0x00; /* X */
    PPU.scroll = 0x00; /* Y */
}

void ppu_finish_vram_update(unsigned char ctrl)
{
    PPU.scroll = 0x00; /* X */
    PPU.scroll = 0x00; /* Y */
    /* Un $2006 de dirección deja el registro interno "t" de la PPU con los
       bits de seleccion de nametable de esa direccion (por ej. $3F00 pone
       esos bits en "3"), y $2005 NO los toca. Solo un $2000 los vuelve a
       poner en los que corresponden. */
    PPU.control = ctrl;
}

void ppu_set_backdrop(unsigned char color)
{
    PPU.vram.address = 0x3F;
    PPU.vram.address = 0x00;
    PPU.vram.data = color;
}
