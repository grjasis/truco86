# Arquitectura de Truco86

## Objetivo y alcance

Truco86 es un juego de Truco para NES, target **PAL-N**, 1 jugador
humano contra 1 CPU. Reglas: Truco (truco/retruco/vale cuatro), Envido
(envido/real envido/falta envido) y Flor/contraflor desde el vamos. Ver
[REGLAS.md](REGLAS.md) para el detalle de reglas.

## Por qué PAL-N

El juego apunta a PAL-N (Paraguay/Uruguay/Argentina). A nivel de ROM esto es
indistinguible de PAL estándar: ambos corren a 50Hz / 50 frames por segundo
(la diferencia PAL vs PAL-N es solo la subportadora de color analógica de la
señal de TV, algo que no vive en el cartucho). Implicancias:

- El header iNES del `.nes` se marca con el flag de TV **PAL** (byte 9, bit 0)
  vía `tools/set_pal_header.py`, que se corre automáticamente después de cada
  build (`make all`).
- Toda la lógica de tiempo del juego (animaciones, debounce de input,
  temporizadores de IA) se basa en contar frames de NMI/vblank, no en
  constantes de "60 FPS" — así el juego anda a la velocidad correcta sea cual
  sea el refresh real de la consola/emulador.
- Se verifica en un emulador con soporte explícito de región PAL/PAL-N (por
  ejemplo Mesen, forzando la región), no alcanza con el modo NTSC por
  defecto de un emulador.

## Toolchain

- **cc65** (`cl65`/`ca65`/`ld65`), instalado vía Homebrew (`brew install
  cc65`). Se usa el target NES incluido en cc65 (`nes.h`, `nes.lib`,
  `cfg/nes.cfg`), no una librería externa tipo "neslib".
- Mapper **NROM (mapper 0)**: 32KB PRG-ROM + 8KB CHR-ROM, definido en
  `nes.cfg` (copia local ajustada de la config por defecto de cc65, con la
  zona de zeropage ampliada para el driver de joystick).
- **Arranque (`src/crt0.s`) propio**, no el `crt0.o` que trae `nes.lib`: ese
  objeto empaqueta 4KB de una fuente de texto por defecto en el mismo
  segmento `CHARS` donde va nuestro CHR de cartas, y como ld65 linkea
  objetos completos, no había forma de aprovechar el resto de ese runtime
  sin arrastrar esos 4KB — dejando solo 4KB libres en un banco de CHR-ROM
  fijo de 8KB. `src/crt0.s` hace el trabajo de arranque a mano (header
  iNES, vectores, init de RAM/stack, copia de `DATA`, NMI mínimo) y sigue
  usando el resto de `nes.lib` (soporte del compilador, driver de
  joystick) con normalidad. Detalle completo en
  [GRAFICOS.md](GRAFICOS.md#por-qué-un-crt0s-propio-nota-importante-de-arquitectura).
- `Makefile` en la raíz regenera el CHR (`tools/make_chr.py`), compila con
  `cl65 -t nes` y corre el parche de header PAL.

## Separación lógica / presentación

El motor de reglas vive en `src/game/` como **C portable sin dependencias de
hardware NES** (sin incluir `nes.h`, sin acceso a PPU/APU). Esto permite
compilarlo también con el compilador C del host (`cc`/`clang`) y testearlo en
segundos con `make test`, sin pasar por un emulador cada vez que se cambia
una regla.

```
src/
  main.c            # boot y loop principal (usa nes.h / nes.lib)
  crt0.s            # arranque propio: header iNES, vectores, init RAM/stack, NMI
  chr_bank.s         # incbin de res/chr/cards.chr en el segmento CHARS
  game/             # LÓGICA PURA — testeable en host
    deck.h/.c        # mazo de 40 cartas, shuffle (xorshift16), numeración real
    card_rank.h/.c    # jerarquía de truco (tabla de 40 entradas, ver REGLAS.md)
    envido.h/.c       # cálculo de puntos de envido
    flor.h/.c         # detección y valor de flor
    match.h/.c        # resultado de baza y de mano (jerarquía + pardas)
    canto.h/.c        # valores "quiero"/"no quiero" de envido, flor y truco
    ai.h/.c           # IA de la CPU: que carta jugar y cuando aceptar/escalar — Fase 7
  platform/         # capa específica NES
    ppu_draw.h/.c     # paleta, limpieza de nametable, escritura de tiles, ppu_finish_vram_update()
    cards_render.h/.c # traduce una Card del motor de reglas a tiles de carta
    vsync.h/.c        # espera de vblank basada en el NMI de crt0.s
    input.h/.c        # lectura de controles con deteccion de flanco (pressed)
    text.h/.c         # texto (subconjunto de letras) y numeros (marcador) para la UI
    canto_ui.h/.c     # menu de cantos de envido/flor — Fase 5
    truco_ui.h/.c     # menu de cantos de truco/retruco/vale cuatro — Fase 6
    sound.h/.c        # SFX de un canal (pulso 1), "tira y olvida" — Fase 9
    music.h/.c        # melodia de un canal (pulso 2) para la pantalla de titulo
  test/
    test_rules.c      # tests de host para src/game/*
    Makefile           # `make test` compila y corre con gcc/clang
res/
  chr/
    cards.chr         # generado por tools/make_chr.py, no editar a mano
tools/
  make_chr.py        # genera res/chr/cards.chr (numeros, palos, dorso, letras)
  preview_render.py  # renderiza un PNG de verificación sin emulador
  set_pal_header.py  # parche post-build del header iNES a PAL
```

## Representación de datos

- **Carta:** un byte `0..39`. `suit = carta / 10` (0=espada, 1=basto, 2=oro,
  3=copa), `rank = carta % 10` (0..6 = As..Siete, 7..9 = 10,11,12). Ver
  `src/game/deck.h`.
- **Jerarquía de truco:** tabla estática `power_table[40]` en
  `src/game/card_rank.c`, poder 1 (más bajo, el 4) a 14 (más alto, 1 de
  espada). `truco_compare()` da el resultado de una baza.
- **Envido/Flor:** `envido_score()` y `flor_score()` operan sobre un arreglo
  de 3 `Card`.

## Gráficos de las cartas

Ver [GRAFICOS.md](GRAFICOS.md): cada carta se compone de 2 tiles (número +
palo) en vez de 40 ilustraciones completas — detalle del pipeline CHR, tabla
de IDs de tile y paleta.

## Estado del proyecto / próximos pasos

Ver el plan de fases completo en el historial de la sesión de diseño.
Completado: motor de reglas (mazo, truco, envido, flor), bootstrap NES con
arranque propio, gráficos de cartas, loop de manos de truco jugadas una
atrás de otra (reparto/turnos/bazas), cantos de envido/flor y de
truco/retruco/vale cuatro con menú interactivo e "irse al mazo", una IA de
la CPU basada en heurísticas (ver
[REGLAS.md](REGLAS.md#ia-de-la-cpu-fase-7)), y un marcador numérico
persistente con condición de fin de partida a 15 o 30 puntos, elegible
desde un menú en la pantalla de inicio junto con jugar con o sin flor
(Fase 8, ver [REGLAS.md](REGLAS.md#partida-fase-8)), sonido básico (Fase 9, ver
[GRAFICOS.md](GRAFICOS.md#sonido-fase-9)), y dos rondas de pulido de
interfaz/interacción (ver
[GRAFICOS.md](GRAFICOS.md#pulido-de-interfaz-e-interacción-después-de-la-fase-8)
y
[GRAFICOS.md](GRAFICOS.md#naipes-verticales-pantalla-de-título-y-cursor-unificado-después-de-la-fase-9)):
etiquetas VOS/CPU, cartas con forma de naipe español (verticales, número
arriba y palo abajo, con marco), mensajes de qué canta la CPU, una grilla
persistente de cartas jugadas (2 filas x 3 columnas, una por baza) para
comparar quién ganó cada una, pantalla de título ("TRUCO 86", créditos a
grjasis@code.ar) antes de la primera mano (con música mínima de un canal,
ver [GRAFICOS.md](GRAFICOS.md#música-de-la-pantalla-de-título)), un
cursor único que combina elegir carta con cantar truco/retruco/vale
cuatro Y envido/real/falta envido (Arriba/Abajo desde la fila de cartas,
sin SELECT ni un "Paso" obligatorio para ninguno de los dos), una regla de
turno para escalar el truco (quien cantó el último escalón aceptado no
puede volver a escalar por su cuenta hasta que el otro lo haga, ver
[REGLAS.md](REGLAS.md#truco)), la mano terminando apenas queda decidida
con 2 de las 3 bazas (sin forzar jugar la 3ra si ya no cambia nada), una
CPU que ya no solo responde (también toma la iniciativa y canta
truco/envido por su cuenta con una mano fuerte), la prioridad real del
envido sobre el truco (mientras un canto de truco no fue aceptado,
cualquiera de los dos puede interrumpir con envido en vez de responder;
una vez aceptado, el envido ya no se puede cantar mas esa mano — ver
[REGLAS.md](REGLAS.md#truco)), quien tira primero en cada baza (gana la
anterior tira primero en la siguiente; parda: sigue el mismo, ver
[REGLAS.md](REGLAS.md#partida-fase-8)), la mano (quien tira/habla primero
en la 1ra baza de cada mano, y gana los empates de envido/flor) rotando
entre el jugador y la CPU una mano tras otra (`hand_mano` en `main.c`, ver
[REGLAS.md](REGLAS.md#partida-fase-8)), y música (de un canal) para ganar
o perder una mano además de la de la pantalla de título. Encima de todo eso
hay una pasada de UI/UX (ver
[GRAFICOS.md](GRAFICOS.md#rediseño-de-interfaz-texto-blanco-barra-de-estado-y-marcas-de-baza)):
texto blanco con sombra en vez de tinta negra sobre el paño, una barra de
estado arriba con el marcador de los dos, cuánto vale la mano (`VALE n`) y de
qué lado está la mano, marcas de quién ganó cada baza sobre una línea de mesa,
cartas centradas en la pantalla, un dorso de carta dibujado (naipe rojo con
rombo) en vez del damero anterior, puntero ancho para elegir carta y flecha
`►` en los menús, y marcos de interfaz en la pantalla de título y en la
final. Lo que falta:

1. Música con más de un canal / distinta melodía durante las manos (hoy
   solo hay melodías de un canal: la de la pantalla de título y los
   jingles de ganar/perder mano).
2. Distinción "malas"/"buenas" (mitad de partida) — hoy `target_score`
   (15 o 30, elegido en la pantalla de inicio) es un número simple sin
   ese matiz.
3. Contraflor/contraflor al resto interactivos cuando ambos jugadores
   tienen flor (hoy se resuelve automático, ver REGLAS.md).
4. IA más avanzada: la CPU es heurística fija (sin memoria de qué cartas ya
   salieron, sin faroles reales más allá del jitter de `entropy`, y no
   puede irse al mazo por su cuenta).

## Cómo compilar y probar

```sh
make test   # corre los tests de reglas en el host (rápido, sin emulador)
make all    # compila build/truco86.nes (PAL-N) con cc65
make run    # recuerda abrir el .nes en un emulador con soporte PAL/PAL-N
```
