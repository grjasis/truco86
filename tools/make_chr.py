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
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
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
    """Letra de la interfaz: fondo transparente (flota sobre el paño), tinta
    BLANCA con una sombra negra corrida 1px abajo/derecha. El blanco sobre el
    verde del paño tiene mucho mas contraste que la tinta negra que se usaba
    antes (se leia apagada), y la sombra le da relieve y la despega del fondo
    sin necesitar un panel opaco atras. La sombra va corrida SOLO a la
    derecha (no hacia abajo): asi el glifo sigue midiendo 7px de alto y queda
    1px libre entre renglones — con la sombra hacia abajo las lineas de los
    menus se tocaban entre si y se leian apelmazadas."""
    g = blank_grid(".")
    stamp(g, FONT_5X7[ch], top=0, left=2, ink="B")
    stamp(g, FONT_5X7[ch], top=0, left=1, ink="W")
    return g


def hud_digit_tile(d):
    """Digito de la interfaz (marcador, opciones del titulo): mismo estilo que
    letter_tile(), fondo transparente. Distinto de big_digit_tile(), que tiene
    fondo BLANCO porque va impreso sobre la cara de una carta."""
    return letter_tile(d)


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
# palos chicos (ESPADA..COPA) quedaron sin uso.
#
# Los CUATRO usan la tinta 'R' (color 3), incluso espada y basto que se ven
# azul y verde: el color 3 es el unico que cambia entre las 4 paletas de palo
# (ver palette[] en main.c). El color 2 queda negro en todas, asi el numero de
# la carta y su marco siguen siendo negros aunque el bloque de atributos que
# pinta el palo tambien los alcance — antes el numero salia del color del palo
# y un "6" de basto quedaba verde claro sobre blanco, casi ilegible.
ESPADA_16 = [
    "WWWWWWWRWWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWRRRRRWWWWWW",
    "WWWWWRRRRRWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWRRRRRRRWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWRRRRRWWWWWW",
    "WWWWWWRRRWWWWWWW",
    "WWWWWWWRWWWWWWWW",
]

BASTO_16 = [
    "WWWWWWRRRRWWWWWW",
    "WWWWWWRRRRWWWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWWRRRRRRWWWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWRRRRRRRRRWWWW",
    "WWWRRRRRRRRRWWWW",
    "WWWWRRRRRRRRWWWW",
    "WWWRRRRRRRRRWWWW",
    "WWWRRRRRRRRRWWWW",
    "WWRRRRRRRRRRWWWW",
    "WWRRRRRRRRRRWWWW",
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

# Dorso de carta CHICO (16x16 = 4 tiles), para la mano tapada de la CPU: un
# naipe rojo con marco negro y un rombo blanco en el medio. Antes el dorso era
# un damero de 1px repetido en 2x2 tiles sin marco, y en pantalla se veia como
# una manchita de ruido en vez de una carta dada vuelta.
CARD_BACK_16 = [
    "BBBBBBBBBBBBBBBB",
    "BRRRRRRRRRRRRRRB",
    "BRRRRRRWWRRRRRRB",
    "BRRRRRWWWWRRRRRB",
    "BRRRRWWWRWWWRRRB",
    "BRRRWWWRRRWWWRRB",
    "BRRWWWRRRRRWWWRB",
    "BRWWWRRRRRRRWWWB",
    "BWWWRRRRRRRRRWWB",
    "BRWWWRRRRRRRWWWB",
    "BRRWWWRRRRRWWWRB",
    "BRRRWWWRRRWWWRRB",
    "BRRRRWWWRWWWRRRB",
    "BRRRRRWWWWRRRRRB",
    "BRRRRRRWWRRRRRRB",
    "BBBBBBBBBBBBBBBB",
]

# Dorso de carta (legado, 8x8): cruz-hachurado simple. Reemplazado por
# CARD_BACK_16; se sigue generando para no correr los IDs de tile.
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

# Cursor (legado): flechita chica hacia arriba. Se sigue generando para no
# correr los IDs de tile ya publicados, pero los menus usan ARROW_RIGHT y la
# seleccion de carta usa el puntero ancho CURSOR_WIDE_L/R.
CURSOR = rows_from_strings([
    "........",
    "...WW...",
    "..WWWW..",
    ".WWWWWW.",
    "...WW...",
    "...WW...",
    "........",
    "........",
])

# Puntero ANCHO de seleccion de carta (16x8, dos tiles): un triangulo grande
# que apunta a la carta elegida desde abajo. Antes era una flechita de 8x8
# sola debajo de una carta de 32px de ancho, y casi no se veia cual estaba
# seleccionada.
CURSOR_WIDE_L = rows_from_strings([
    "......BW",
    ".....BWW",
    "....BWWW",
    "...BWWWW",
    "..BWWWWW",
    "........",
    "........",
    "........",
])
CURSOR_WIDE_R = rows_from_strings([
    "WB......",
    "WWB.....",
    "WWWB....",
    "WWWWB...",
    "WWWWWB..",
    "........",
    "........",
    "........",
])

# Cursor de menu: flecha hacia la derecha, apunta al texto de la opcion.
ARROW_RIGHT = rows_from_strings([
    "........",
    "..WB....",
    "..WWB...",
    "..WWWB..",
    "..WWWWB.",
    "..WWWB..",
    "..WWB...",
    "..WB....",
])

# Marcas de resultado de cada baza (se dibujan al costado de la columna de la
# baza): triangulo hacia abajo = la gano el jugador, hacia arriba = la gano la
# CPU, "=" = parda. Asi el 1-0 / 1-1 de la mano se ve de un vistazo, en vez de
# tener que acordarse del flash de color.
MARK_DOWN = rows_from_strings([
    "........",
    ".WWWWWW.",
    ".WWWWWW.",
    "..WWWW..",
    "..WWWW..",
    "...WW...",
    "...WW...",
    "........",
])
MARK_UP = rows_from_strings([
    "........",
    "...WW...",
    "...WW...",
    "..WWWW..",
    "..WWWW..",
    ".WWWWWW.",
    ".WWWWWW.",
    "........",
])
MARK_TIE = rows_from_strings([
    "........",
    "........",
    ".WWWWWW.",
    ".WWWWWW.",
    "........",
    ".WWWWWW.",
    ".WWWWWW.",
    "........",
])

# Marco de la interfaz (linea blanca fina sobre el paño, fondo transparente),
# para encuadrar la pantalla de titulo y la de resultado. Distinto de los
# BORDER_* de arriba, que son el marco NEGRO sobre BLANCO de una carta.
UI_H = rows_from_strings([
    "........",
    "........",
    "........",
    "WWWWWWWW",
    "........",
    "........",
    "........",
    "........",
])
UI_VL = rows_from_strings([
    "...W....",
    "...W....",
    "...W....",
    "...W....",
    "...W....",
    "...W....",
    "...W....",
    "...W....",
])
UI_VR = rows_from_strings([
    "....W...",
    "....W...",
    "....W...",
    "....W...",
    "....W...",
    "....W...",
    "....W...",
    "....W...",
])
UI_TL = rows_from_strings([
    "........",
    "........",
    "........",
    "...WWWWW",
    "...W....",
    "...W....",
    "...W....",
    "...W....",
])
UI_TR = rows_from_strings([
    "........",
    "........",
    "........",
    "WWWWW...",
    "....W...",
    "....W...",
    "....W...",
    "....W...",
])
UI_BL = rows_from_strings([
    "...W....",
    "...W....",
    "...W....",
    "...WWWWW",
    "........",
    "........",
    "........",
    "........",
])
UI_BR = rows_from_strings([
    "....W...",
    "....W...",
    "....W...",
    "WWWWW...",
    "........",
    "........",
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

# Digitos de la INTERFAZ (fondo transparente, blancos con sombra): marcador,
# opciones del titulo, "VALE n". Los big_digit_tile() de arriba (fondo blanco)
# se siguen usando solo para el numero impreso en la cara de una carta.
for _d in "0123456789":
    TILES.append(hud_digit_tile(_d))  # TILE_HUD_DIGIT_BASE + d

TILES.extend(split_quadrants(CARD_BACK_16))  # TILE_BACK_BASE: TL,TR,BL,BR

TILES.append(letter_tile("M"))   # TILE_LETTER_M ("MANO")
TILES.append(MARK_DOWN)          # TILE_MARK_PLAYER
TILES.append(MARK_UP)            # TILE_MARK_CPU
TILES.append(MARK_TIE)           # TILE_MARK_TIE
TILES.append(CURSOR_WIDE_L)      # TILE_CURSOR_WIDE_L
TILES.append(CURSOR_WIDE_R)      # TILE_CURSOR_WIDE_R
TILES.append(ARROW_RIGHT)        # TILE_ARROW_RIGHT
TILES.append(UI_H)               # TILE_UI_H
TILES.append(UI_VL)              # TILE_UI_VL
TILES.append(UI_VR)              # TILE_UI_VR
TILES.append(UI_TL)              # TILE_UI_TL
TILES.append(UI_TR)              # TILE_UI_TR
TILES.append(UI_BL)              # TILE_UI_BL
TILES.append(UI_BR)              # TILE_UI_BR

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
