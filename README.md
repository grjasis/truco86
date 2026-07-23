# Truco86

> **Note (English):** All documentation and code comments in this project are
> written in Spanish, since Truco is an Ibero-American card game played
> mostly by Spanish speakers. A future phase may add translations to other
> languages depending on the project's reception and community forks. For
> now, please use Claude to translate or to get help navigating the Spanish
> docs/comments.

Truco para NES (target **PAL-N**), 1 jugador contra la CPU, con
reglas completas: Truco (truco/retruco/vale cuatro), Envido (envido/real
envido/falta envido) y Flor/contraflor.

Créditos: **grjasis@code.ar**

## Requisitos

- [cc65](https://cc65.github.io/) (`brew install cc65`)
- Python 3 (para el parche de header PAL post-build)
- Un emulador de NES con soporte de región PAL/PAL-N para probar el `.nes`
  (recomendado: [Mesen](https://www.mesen.ca/))

## Compilar y correr

```sh
make test   # tests de las reglas del juego, corren en el host (sin emulador)
make all    # compila build/truco86.nes
```

Abrí `build/truco86.nes` en un emulador forzando región PAL/PAL-N.

## Estructura del proyecto

```
src/
  main.c       # boot y loop principal (NES: PPU/input)
  crt0.s       # arranque propio (header iNES, vectores, init RAM, NMI)
  chr_bank.s   # embebe res/chr/cards.chr en el ROM
  game/        # motor de reglas puro en C portable, testeable en el host
  platform/    # capa específica de hardware NES (PPU, cartas, vsync)
  test/        # tests de host para src/game/
res/chr/       # res/chr/cards.chr, generado por tools/make_chr.py
tools/         # scripts de build (CHR, parche de header PAL, preview PNG)
docs/          # documentación del proyecto
```

## Documentación

- [docs/REGLAS.md](docs/REGLAS.md) — reglas de Truco/Envido/Flor tal como se
  implementan, con la tabla de jerarquía de cartas.
- [docs/ARQUITECTURA.md](docs/ARQUITECTURA.md) — arquitectura técnica,
  toolchain, decisión de PAL-N, estructura de código y roadmap de fases.
- [docs/GRAFICOS.md](docs/GRAFICOS.md) — cómo están hechos los gráficos de
  las cartas, tabla de tiles y por qué el proyecto usa un `crt0.s` propio.

## Verificación visual sin emulador

`python3 tools/preview_render.py build/preview.png` genera un PNG con lo
mismo que dibuja `main.c` en el nametable (útil durante el desarrollo). No
reemplaza probar el `.nes` en un emulador real con soporte PAL-N.

## Estado

Jugable de punta a punta: pantalla de título ("TRUCO 86", créditos a
grjasis@code.ar, con una melodía de un canal inspirada en "La Cumparsita")
con un menú de 4 opciones para elegir las reglas de la partida (CON/SIN
FLOR, A 15/30 puntos) antes de arrancar, manos repartidas una atrás de otra,
con cantos de envido/flor
y de truco/retruco/vale cuatro (con "irse al mazo") integrados en un solo
cursor: parado en las cartas, Arriba sube al menú de canto (envido y/o
truco, lo que haya disponible en ese momento) y Abajo desde la primera
opción vuelve a elegir carta — no hace falta "pasar" nada para jugar
directo ni para saltear el envido. Una CPU con IA propia que además dice
qué cantó ("CPU: QUIERO") y que puede tomar la iniciativa y cantar
truco/envido por su cuenta con una mano fuerte (no solo responder), SFX
cortos para cursor/cartas/cantos/resultado, y un marcador ("VOS"/"CPU")
siempre visible arriba de la pantalla hasta que alguno llega al objetivo
elegido (15 o 30 puntos, pantalla de "GANASTE"/"PERDISTE" al final). Las cartas tienen forma de
naipe español de verdad (verticales, número arriba y palo abajo, con
marco), las jugadas quedan en una grilla persistente (CPU arriba, jugador
abajo, una columna por baza) para poder comparar qué carta ganó cada una,
y la mano termina apenas queda decidida con 2 de las 3 bazas (no fuerza
jugar la 3ra si ya no cambia nada). Ver
[docs/GRAFICOS.md](docs/GRAFICOS.md#naipes-verticales-pantalla-de-título-y-cursor-unificado-después-de-la-fase-9)
para el detalle de estos cambios y [docs/ARQUITECTURA.md](docs/ARQUITECTURA.md)
para lo que falta (música con más de un canal, IA más avanzada, etc.).

## Licencia

[MIT](LICENSE) — Gustavo Riveros Jasis \<grjasis@gmail.com\>.
