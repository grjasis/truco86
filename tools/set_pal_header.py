#!/usr/bin/env python3
"""Marca el header iNES de un .nes como PAL (PAL-N usa la misma temporizacion
de 50Hz que PAL estandar; solo difiere la subportadora de color analogica de
la TV, que no es un dato que viva en el header del ROM).

Uso: set_pal_header.py archivo.nes
"""
import sys

def main():
    if len(sys.argv) != 2:
        sys.exit("uso: set_pal_header.py archivo.nes")

    path = sys.argv[1]
    with open(path, "rb") as f:
        data = bytearray(f.read())

    if data[0:4] != b"NES\x1a":
        sys.exit(f"{path}: no parece un header iNES valido")

    # Byte 9, bit 0: 0 = NTSC, 1 = PAL (iNES 1.0 TV system flag)
    data[9] |= 0x01

    with open(path, "wb") as f:
        f.write(data)

    print(f"{path}: header marcado como PAL (PAL-N usa la misma temporizacion 50Hz)")

if __name__ == "__main__":
    main()
