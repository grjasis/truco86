#!/usr/bin/env python3
"""Genera res/chr/cards.chr: el banco de 8KB de CHR-ROM con los tiles de
Truco86 (numeros de carta, palos y dorso), en formato NES 2bpp planar.

Cada tile se define como arte ASCII de 8x8 en TILES[] mas abajo:
  '.' = color 0 (transparente / color de fondo universal)
  'W' = color 1 (blanco, relleno de la carta)
  'B' = color 2 (negro, tinta de espada/basto y numeros)
  'R' = color 3 (rojo, tinta de oro/copa)

El orden de TILES define el ID de tile (0, 1, 2, ...). Ver
docs/GRAFICOS.md para la tabla de IDs y como se usan desde
src/platform/cards_render.c.
"""
import sys

TILE_SIZE = 8
CHR_BANK_SIZE = 8192  # 8KB, un solo banco de patrones fijo (mapper NROM)

# ---------------------------------------------------------------------------
# Fuente compacta 3x5 para los digitos 0,1,2 (se usan para armar "10","11","12"
# dentro de un solo tile de 8x8, dos digitos angostos lado a lado).
FONT_3X5 = {
    "0": ["111", "101", "101", "101", "111"],
    "1": ["010", "110", "010", "010", "111"],
    "2": ["111", "001", "111", "100", "111"],
}

# Fuente grande 5x7 para los digitos sueltos 1..7 (numeros de carta que no
# necesitan un segundo digito) y para las letras mayusculas que usa el menu
# de cantos (ENVIDO, REAL, FALTA, FLOR, QUIERO, PASO, NO).
FONT_5X7 = {
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "10001", "01110"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "C": ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "G": ["01111", "10000", "10000", "10011", "10001", "10001", "01110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "N": ["10001", "11001", "10101", "10101", "10011", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "Q": ["01110", "10001", "10001", "10001", "10101", "10011", "01111"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    ".": ["00000", "00000", "00000", "00000", "00000", "01100", "01100"],
}


def blank_grid(bg="W"):
    return [[bg for _ in range(TILE_SIZE)] for _ in range(TILE_SIZE)]


def stamp(grid, pattern, top, left, ink="B"):
    for r, row in enumerate(pattern):
        for c, bit in enumerate(row):
            if bit == "1":
                grid[top + r][left + c] = ink


def big_digit_tile(d):
    g = blank_grid("W")
    stamp(g, FONT_5X7[d], top=0, left=1, ink="B")
    return g


def letter_tile(ch):
    """Letra de menu: fondo transparente (flota sobre el paño), tinta negra."""
    g = blank_grid(".")
    stamp(g, FONT_5X7[ch], top=0, left=1, ink="B")
    return g


def double_digit_tile(tens, ones):
    g = blank_grid("W")
    stamp(g, FONT_3X5[tens], top=1, left=0, ink="B")
    stamp(g, FONT_3X5[ones], top=1, left=4, ink="B")
    return g


def rows_from_strings(rows):
    return [list(r) for r in rows]


def split_quadrants(pattern16):
    """Parte un patron de 16x16 (lista de 16 strings de 16 caracteres, con
    los codigos de color W/B/R/. directos) en 4 tiles de 8x8: arriba-izq,
    arriba-der, abajo-izq, abajo-der (en ese orden)."""
    rows = [list(r) for r in pattern16]
    tl = [row[0:8] for row in rows[0:8]]
    tr = [row[8:16] for row in rows[0:8]]
    bl = [row[0:8] for row in rows[8:16]]
    br = [row[8:16] for row in rows[8:16]]
    return tl, tr, bl, br


# ---------------------------------------------------------------------------
# Palos (8x8), diseño minimalista: forma + color de tinta distingue cada palo.
ESPADA = rows_from_strings([
    "WWWWWWWW",
    "WWWWWWBW",
    "WWWWWBWW",
    "WWWWBWWW",
    "WWWBWWWW",
    "WWBWWWWW",
    "WBWWWWWW",
    "WWWWWWWW",
])

BASTO = rows_from_strings([
    "WWWBBWWW",
    "WWWBBWWW",
    "WWBBBBWW",
    "WWWBBWWW",
    "WWWBBWWW",
    "WWWBBWWW",
    "WWBBBBWW",
    "WWWWWWWW",
])

ORO = rows_from_strings([
    "WWRRRRWW",
    "WRRRRRRW",
    "RRRWWRRR",
    "RRRWWRRR",
    "RRRWWRRR",
    "WRRRRRRW",
    "WWRRRRWW",
    "WWWWWWWW",
])

COPA = rows_from_strings([
    "RWWWWWRW",
    "RWWWWWRW",
    "WRWWWRWW",
    "WWRRRWWW",
    "WWWRWWWW",
    "WWRRRWWW",
    "WRRRRRWW",
    "WWWWWWWW",
])

# ---------------------------------------------------------------------------
# Palos GRANDES (16x16, partidos en 4 tiles de 8x8 con split_quadrants):
# version mas grande y detallada de los palos de arriba, para el numero/palo
# de cada carta en mesa (ver cards_render.c, TILE_SUIT_*_TL/TR/BL/BR). Los
# palos chicos (ESPADA..COPA) se siguen usando nada mas para el adorno de la
# pantalla de titulo.
ESPADA_16 = [
    "WWWWWWWBWWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWBBBBBWWWWWW",
    "WWWWWBBBBBWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWBBBBBBBWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWBBBBBWWWWWW",
    "WWWWWWBBBWWWWWWW",
    "WWWWWWWBWWWWWWWW",
]

BASTO_16 = [
    "WWWWWWBBBBWWWWWW",
    "WWWWWWBBBBWWWWWW",
    "WWWWWBBBBBBWWWWW",
    "WWWWWBBBBBBWWWWW",
    "WWWWBBBBBBBBWWWW",
    "WWWWWBBBBBBWWWWW",
    "WWWWWBBBBBBWWWWW",
    "WWWWBBBBBBBBWWWW",
    "WWWWBBBBBBBBWWWW",
    "WWWBBBBBBBBBWWWW",
    "WWWBBBBBBBBBWWWW",
    "WWWWBBBBBBBBWWWW",
    "WWWBBBBBBBBBWWWW",
    "WWWBBBBBBBBBWWWW",
    "WWBBBBBBBBBBWWWW",
    "WWBBBBBBBBBBWWWW",
]

# Disco solido (moneda/sol): mas claro a este tamano que un anillo con
# hueco (se podia confundir con una rosquilla/letra O).
ORO_16 = [
    "WWWWRRRRRRRRWWWW",
    "WWRRRRRRRRRRRRWW",
    "WRRRRRRRRRRRRRRW",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "WRRRRRRRRRRRRRRW",
    "WWRRRRRRRRRRRRWW",
    "WWWWRRRRRRRRWWWW",
    "WWWWWWWWWWWWWWWW",
]

# Copa/caliz solido (silueta llena en vez de contorno fino): una copa
# ancha arriba que se angosta en una pata y una base, se entiende mejor
# de un vistazo a este tamano que un dibujo de lineas.
COPA_16 = [
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "WRRRRRRRRRRRRRRW",
    "WWRRRRRRRRRRRRWW",
    "WWWRRRRRRRRRRWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWWWRRRRWWWWWW",
    "WWWWWWWRRWWWWWWW",
    "WWWWWWWRRWWWWWWW",
    "WWWWWWWRRWWWWWWW",
    "WWWWWWRRRRWWWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWWWWWWWWWWWWWW",
    "WWWWWWWWWWWWWWWW",
]

# Dorso de carta: cruz-hachurado simple, para las cartas boca abajo de la CPU.
CARD_BACK = rows_from_strings([
    "BWBWBWBW",
    "WBWBWBWB",
    "BWBWBWBW",
    "WBWBWBWB",
    "BWBWBWBW",
    "WBWBWBWB",
    "BWBWBWBW",
    "WBWBWBWB",
])

# Cursor: flechita hacia arriba, se dibuja debajo de la carta seleccionada.
CURSOR = rows_from_strings([
    "........",
    "...BB...",
    "..BBBB..",
    ".BBBBBB.",
    "...BB...",
    "...BB...",
    "........",
    "........",
])


# Marco de carta (para la mano del jugador): esquinas + bordes de 1px negro
# sobre blanco, para que la carta se vea como una carta de verdad y no un
# numero flotando en el paño. Se dibujan alrededor de [numero][palo] con
# draw_card_border() (src/platform/cards_render.c).
BORDER_TOP = rows_from_strings([
    "BBBBBBBB",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
])
BORDER_BOTTOM = rows_from_strings([
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "WWWWWWWW",
    "BBBBBBBB",
])
BORDER_LEFT = rows_from_strings([
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
])
BORDER_RIGHT = rows_from_strings([
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
])
BORDER_TL = rows_from_strings([
    "BBBBBBBB",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
])
BORDER_TR = rows_from_strings([
    "BBBBBBBB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
])
BORDER_BL = rows_from_strings([
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BWWWWWWW",
    "BBBBBBBB",
])
BORDER_BR = rows_from_strings([
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "WWWWWWWB",
    "BBBBBBBB",
])


def tile0():
    return blank_grid(".")  # tile 0: todo color 0 (transparente), para "limpiar" el nametable


# ---------------------------------------------------------------------------
# Tabla de tiles, en orden = ID de tile. Mantener sincronizado con
# src/platform/cards_render.h (TILE_* defines) y docs/GRAFICOS.md.
TILES = [
    tile0(),                              # 0: blanco/transparente (limpieza de nametable)
    big_digit_tile("1"),                  # 1
    big_digit_tile("2"),                  # 2
    big_digit_tile("3"),                  # 3
    big_digit_tile("4"),                  # 4
    big_digit_tile("5"),                  # 5
    big_digit_tile("6"),                  # 6
    big_digit_tile("7"),                  # 7
    double_digit_tile("1", "0"),          # 8: "10"
    double_digit_tile("1", "1"),          # 9: "11"
    double_digit_tile("1", "2"),          # 10: "12"
    ESPADA,                                # 11
    BASTO,                                 # 12
    ORO,                                   # 13
    COPA,                                  # 14
    CARD_BACK,                             # 15
    CURSOR,                                 # 16
    letter_tile("A"),                      # 17
    letter_tile("D"),                      # 18
    letter_tile("E"),                      # 19
    letter_tile("F"),                      # 20
    letter_tile("I"),                      # 21
    letter_tile("L"),                      # 22
    letter_tile("N"),                      # 23
    letter_tile("O"),                      # 24
    letter_tile("P"),                      # 25
    letter_tile("Q"),                      # 26
    letter_tile("R"),                      # 27
    letter_tile("S"),                      # 28
    letter_tile("T"),                      # 29
    letter_tile("U"),                      # 30
    letter_tile("V"),                      # 31
    letter_tile("C"),                      # 32
    big_digit_tile("0"),                   # 33
    big_digit_tile("8"),                   # 34
    big_digit_tile("9"),                   # 35
    letter_tile("G"),                      # 36
    BORDER_TOP,                             # 37
    BORDER_BOTTOM,                          # 38
    BORDER_LEFT,                            # 39
    BORDER_RIGHT,                           # 40
    BORDER_TL,                              # 41
    BORDER_TR,                              # 42
    BORDER_BL,                              # 43
    BORDER_BR,                              # 44
]

# Palos grandes (16x16 -> 4 tiles de 8x8 cada uno), en el mismo orden de
# palos que los chicos (espada, basto, oro, copa): TL,TR,BL,BR consecutivos,
# ver TILE_SUIT_BIG_BASE en cards_render.h.
for _pattern in (ESPADA_16, BASTO_16, ORO_16, COPA_16):
    TILES.extend(split_quadrants(_pattern))

# Blanco solido (a diferencia del tile 0, que es TRANSPARENTE/color de
# fondo universal): para "no hay nada aca" sobre fondo blanco de carta
# (el digito de las decenas cuando el numero es de 1 solo digito, ver
# rank_tiles_for() en cards_render.c). Usar el tile 0 ahi dejaba un
# cuadrado verde (el color de fondo del paño) en el medio de la carta
# blanca en vez de blanco liso.
TILES.append(blank_grid("W"))  # TILE_WHITE

TILES.append(letter_tile("."))  # TILE_LETTER_DOT: para "CODE.AR" en la pantalla de titulo

COLOR_INDEX = {".": 0, "W": 1, "B": 2, "R": 3}


def pack_tile(grid):
    """2bpp planar NES: 8 bytes plano bajo + 8 bytes plano alto."""
    lo = bytearray(8)
    hi = bytearray(8)
    for r in range(8):
        for c in range(8):
            v = COLOR_INDEX[grid[r][c]]
            bit = 7 - c
            if v & 0x01:
                lo[r] |= (1 << bit)
            if v & 0x02:
                hi[r] |= (1 << bit)
    return bytes(lo) + bytes(hi)


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else "res/chr/cards.chr"

    data = bytearray()
    for tile in TILES:
        data += pack_tile(tile)

    if len(data) > CHR_BANK_SIZE:
        sys.exit(f"error: {len(data)} bytes de tiles exceden el banco CHR de {CHR_BANK_SIZE}")

    data += bytes(CHR_BANK_SIZE - len(data))  # padding a 8KB

    with open(out_path, "wb") as f:
        f.write(data)

    print(f"{out_path}: {len(TILES)} tiles definidos, {CHR_BANK_SIZE} bytes escritos")


if __name__ == "__main__":
    main()
