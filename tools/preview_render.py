#!/usr/bin/env python3
"""Renderiza en un PNG lo que main.c dibuja en el nametable, sin necesitar
un emulador de NES. Replica a mano: el PRNG/shuffle de src/game/deck.c, el
mapeo de rank/suit a tile de src/platform/cards_render.c, y el layout de
src/main.c. Solo para verificacion visual rapida durante el desarrollo,
no reemplaza probar el .nes en un emulador real.

Uso: preview_render.py [salida.png]
"""
import struct
import sys
import zlib

CHR_PATH = "res/chr/cards.chr"
TILE_COUNT = 16

# Paleta de fondo 0 tal como se carga en main.c (palette[]), aproximada a RGB.
NES_RGB = {
    0x1A: (12, 92, 30),     # backdrop: verde pano
    0x30: (255, 255, 255),  # blanco (relleno de carta)
    0x0F: (10, 10, 10),     # negro (tinta)
    0x16: (172, 46, 40),    # rojo (tinta)
}
PALETTE = [0x1A, 0x30, 0x0F, 0x16]


def load_tiles(path):
    with open(path, "rb") as f:
        data = f.read()
    tiles = []
    for t in range(TILE_COUNT):
        base = t * 16
        lo = data[base:base + 8]
        hi = data[base + 8:base + 16]
        grid = []
        for r in range(8):
            row = []
            for c in range(8):
                bit = 7 - c
                v = ((lo[r] >> bit) & 1) | (((hi[r] >> bit) & 1) << 1)
                row.append(v)
            grid.append(row)
        tiles.append(grid)
    return tiles


# --- replica de src/game/deck.c ---
DECK_SIZE = 40
rng_state = 1


def rng_next():
    global rng_state
    rng_state ^= (rng_state << 7) & 0xFFFF
    rng_state ^= (rng_state >> 9) & 0xFFFF
    rng_state ^= (rng_state << 8) & 0xFFFF
    return rng_state & 0xFFFF


def deck_init_shuffled(seed):
    global rng_state
    rng_state = seed if seed else 1
    deck = list(range(DECK_SIZE))
    for i in range(DECK_SIZE - 1, 0, -1):
        j = rng_next() % (i + 1)
        deck[i], deck[j] = deck[j], deck[i]
    return deck


def card_number(c):
    table = [1, 2, 3, 4, 5, 6, 7, 10, 11, 12]
    return table[c % 10]


def card_suit(c):
    return c // 10


# --- replica de src/platform/cards_render.c ---
TILE_RANK_1 = 1
TILE_RANK_10 = 8
TILE_RANK_11 = 9
TILE_RANK_12 = 10
TILE_SUIT_ESPADA = 11
TILE_CARD_BACK = 15


def rank_tile_for(c):
    n = card_number(c)
    if n <= 7:
        return TILE_RANK_1 + (n - 1)
    return {10: TILE_RANK_10, 11: TILE_RANK_11, 12: TILE_RANK_12}[n]


def suit_tile_for(c):
    return TILE_SUIT_ESPADA + card_suit(c)


# --- replica del layout de src/main.c ---
HAND_COL = [4, 12, 20]
PLAYER_ROW = 22
CPU_ROW = 4


def build_nametable():
    nt = [[0] * 32 for _ in range(30)]
    deck = deck_init_shuffled(20260722)
    for i in range(3):
        col = HAND_COL[i]
        card = deck[i]
        nt[PLAYER_ROW][col] = rank_tile_for(card)
        nt[PLAYER_ROW][col + 1] = suit_tile_for(card)
        nt[CPU_ROW][col] = TILE_CARD_BACK
        nt[CPU_ROW][col + 1] = TILE_CARD_BACK
    return nt


def render_png(path):
    tiles = load_tiles(CHR_PATH)
    nt = build_nametable()

    width, height = 32 * 8, 30 * 8
    rgb_pal = [NES_RGB[v] for v in PALETTE]

    rows = []
    for py in range(height):
        row_bytes = bytearray([0])  # filtro PNG: none
        ty, ry = divmod(py, 8)
        for px in range(width):
            tx, rx = divmod(px, 8)
            tile_id = nt[ty][tx]
            color_idx = tiles[tile_id][ry][rx]
            row_bytes += bytes(rgb_pal[color_idx])
        rows.append(bytes(row_bytes))

    raw = b"".join(rows)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", ihdr)
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")

    with open(path, "wb") as f:
        f.write(png)
    print(f"{path}: {width}x{height}px")


if __name__ == "__main__":
    render_png(sys.argv[1] if len(sys.argv) > 1 else "build/preview.png")
