# Gráficos de Truco86 (Fases 3-9 + pulido de interfaz)

## Enfoque

En vez de dibujar las 40 cartas completas como ilustraciones (caro en tiempo
de arte y en CHR-ROM), cada carta se compone en pantalla con **2 tiles de
8x8 apilados verticalmente** (número arriba, palo abajo, misma columna) más
un marco de 1px alrededor, para que la silueta sea más alta que ancha —
como un naipe español de verdad, no un rectángulo horizontal:

```
┌───┐
│ 3 │
│ ♥ │
└───┘
```

- El **número** (1..7, o "10"/"11"/"12" comprimido en un solo tile) se
  genera con una fuente bitmap simple.
- El **palo** es un ícono minimalista de 8x8: espada y basto en tinta negra,
  oro y copa en tinta roja (para que a simple vista, aun a baja resolución,
  se distingan los 4 palos por forma y color).
- El **marco** (8 tiles: 4 esquinas + 4 aristas, ver tabla más abajo) se
  dibuja con `draw_card_border()` alrededor de toda carta boca arriba (mano
  del jugador y las cartas ya jugadas); las cartas de la CPU boca abajo no
  llevan marco, solo el tile de dorso (patrón cruz-hachurado) repetido dos
  veces verticalmente.

Este esquema usa apenas ~20 tiles del banco de 8KB de CHR-ROM (más las
letras/dígitos de la interfaz), así que hay muchísimo margen para más
detalle de arte en fases posteriores sin quedarse sin espacio.

## Generación del CHR

`tools/make_chr.py` define cada tile como arte ASCII de 8x8 (`.`=color 0,
`W`=color1 blanco, `B`=color2 negro, `R`=color3 rojo) y lo empaqueta al
formato NES 2bpp planar en `res/chr/cards.chr` (8KB, con padding a cero). El
`Makefile` lo regenera automáticamente antes de cada build (`make chr` /
`make all`).

`src/chr_bank.s` mete ese archivo tal cual en el segmento `CHARS` del ROM
(`.incbin`), que el linker (`nes.cfg`) ubica en el banco fijo de 8KB de
CHR-ROM.

## Tabla de IDs de tile

| ID | Contenido |
|----|-----------|
| 0  | Blanco/transparente (tile "vacío", usado para limpiar el nametable) |
| 1-7 | Dígito grande 1..7 |
| 8  | "10" |
| 9  | "11" |
| 10 | "12" |
| 11 | Palo espada (tinta negra) |
| 12 | Palo basto (tinta negra) |
| 13 | Palo oro (tinta roja) |
| 14 | Palo copa (tinta roja) |
| 15 | Dorso de carta |
| 16 | Cursor (flechita, se dibuja debajo de la carta seleccionada o al lado de una opción de menú) |
| 17-31 | Letras mayúsculas A,D,E,F,I,L,N,O,P,Q,R,S,T,U,V (fondo transparente, tinta negra) — subconjunto para ENVIDO, REAL, FALTA, FLOR, QUIERO, NO, PASO |
| 32 | Letra C (fondo transparente, tinta negra) — para TRUCO, RETRUCO, CUATRO |
| 33-35 | Dígitos grandes 0, 8, 9 (fondo blanco, igual estilo que 1-7) — para el marcador, que puede necesitar cualquier dígito 0-9 |
| 36 | Letra G (fondo transparente) — para GANASTE |
| 37-40 | Borde de carta: arista superior, inferior, izquierda, derecha (1px negro sobre blanco) |
| 41-44 | Borde de carta: esquinas superior-izq., superior-der., inferior-izq., inferior-der. |

Estas constantes están espejadas en `src/platform/cards_render.h`
(`TILE_RANK_1`, `TILE_SUIT_ESPADA`, `TILE_LETTER_A`, `TILE_DIGIT_0`,
`TILE_BORDER_TL`, etc.). Si se cambia el orden en `tools/make_chr.py`, hay
que actualizar ese header (y esta tabla) a la vez. `src/platform/text.c`
(`letter_tile()`, `digit_tile()`) mapea caracteres/dígitos a estos tiles
para dibujar texto y números (ver "Menú de cantos" y "Marcador" más abajo).

## Paleta

Una sola paleta de fondo alcanza para todos los tiles de carta:

| Índice | Color | Uso |
|--------|-------|-----|
| 0 | `$1A` (verde paño) | color universal de fondo |
| 1 | `$30` (blanco) | relleno de la carta |
| 2 | `$0F` (negro) | tinta de espada/basto y números |
| 3 | `$16` (rojo) | tinta de oro/copa |

Definida en `main.c` (`palette[]`) y cargada con `ppu_load_palette()`
(`src/platform/ppu_draw.c`).

## Cómo se conecta con el motor de reglas

`src/platform/cards_render.c` traduce una `Card` del motor de reglas
(`src/game/deck.h`) a tiles:

- `rank_tile_for(c)` usa `card_number(c)` (ya definido en `deck.c`) para
  elegir el tile de número correcto.
- `suit_tile_for(c)` usa `CARD_SUIT(c)` para elegir el tile de palo (el
  orden espada/basto/oro/copa coincide 1:1 con el orden de los tiles de palo
  generados, ver la tabla arriba).

Así, `main.c` reparte un mazo real (`deck_init_shuffled`) y lo dibuja sin
tener que traducir manualmente cartas a gráficos en ningún otro lugar.

## Por qué un `crt0.s` propio (nota importante de arquitectura)

El `crt0.o` que trae `nes.lib` (el runtime de arranque que usa cc65 para el
target NES) incluye, empaquetados en el mismo objeto, 4KB de una fuente de
texto por defecto en el segmento `CHARS`. Como ld65 linkea objetos completos
o nada, no había forma de usar el resto de ese runtime (inicialización de
RAM, vectores, etc.) sin arrastrar esos 4KB — y en un banco de CHR-ROM fijo
de 8KB para NROM, eso no dejaba espacio para nuestros propios tiles de
cartas.

La solución (estándar en desarrollo de NES con cc65, ver
`docs/ARQUITECTURA.md`) fue escribir un `crt0.s` propio (`src/crt0.s`):
header iNES, vectores de interrupción, inicialización de RAM/stack, copia de
datos inicializados y un manejador de NMI mínimo que solo cuenta frames
(`nes_frame_count`, usado por `src/platform/vsync.c` en vez de la
`waitvsync()` de la librería, que dependía del NMI del `crt0.o` original).
El resto del runtime de cc65 (soporte del compilador, driver de joystick)
se sigue usando normalmente desde `nes.lib`.

## Bug real: pantalla en blanco/glitcheada al jugar (ya resuelto)

Al probar el ROM en un emulador real (fceux) apareció una pantalla sólida
de un solo color en vez de las cartas — a pesar de que el armado del
nametable durante el reparto (con el render apagado) era correcto. Se
reprodujo y aisló con capturas de pantalla automatizadas por Lua en fceux
(ver `tools/preview_render.py` para la verificación estática; esto requirió
la cosa real corriendo).

Causa: durante el juego (cursor, cartas jugadas, cambio de color al
terminar la mano) se escribe VRAM ($2006/$2007) con el render ya prendido.
Aunque esas escrituras se hacían dentro de la ventana de vblank (justo
después de `wait_vblank()`), un `$2006` de dirección pisa, en el registro
interno **"t"** de la PPU, los bits de qué nametable se usa — y esos bits
**no** se reponen escribiendo `$2005` (scroll), solo escribiendo `$2000`
(`PPU_CTRL`) de nuevo. Sin eso, el resto de los frames se renderizaban
desde un nametable que nunca se llenó, mostrando solo el color de fondo.

Arreglo: `ppu_finish_vram_update(ctrl)` en `src/platform/ppu_draw.c` repone
scroll (0,0) **y** reescribe `PPU_CTRL`; hay que llamarla después de
cualquier tanda de escrituras a VRAM hecha con el render prendido (todo
`src/main.c` ya lo hace). Antes de prender el render por primera vez no
hace falta.

## Menú de cantos (Fase 5)

`src/platform/canto_ui.c` implementa un menú vertical simple (texto +
cursor) reutilizando `text.c` y las mismas reglas de escritura de VRAM que
el resto del juego (`wait_vblank()` + `ppu_finish_vram_update()` en cada
tanda). El jugador navega las opciones con Arriba/Abajo y confirma con A;
la CPU responde con la IA de `src/game/ai.c` (Fase 7). Los puntos se
calculan con el motor puro `src/game/canto.c` (ver `docs/REGLAS.md`), se
suman al marcador persistente (Fase 8, ver más abajo) y ademas se resaltan
con un flash de color de fondo (`flash_backdrop()` en `main.c`).

## Menú de truco (Fase 6)

`src/platform/truco_ui.c` es hermano de `canto_ui.c`: mismo patrón de menú
vertical con texto + cursor, mismas reglas de escritura de VRAM. Se ofrece
al principio del turno del jugador en cada baza (antes de elegir carta):
Truco/Retruco/Vale Cuatro (según lo que sea legal cantar), Irse al mazo, o
Paso (seguir jugando sin cantar nada). Si el jugador canta algo, la CPU
responde o escala con la IA de `src/game/ai.c` (Fase 7) basada en el poder
de truco (`card_rank.h`) de su mejor carta sin jugar todavía. Si el canto
termina en "no quiero" o alguien se va al mazo, la mano termina ahí mismo
(no se juegan las bazas que faltaban) y `main.c` lo muestra con el mismo
flash de color que el resto de los resultados.

## IA de la CPU (Fase 7)

Ver [REGLAS.md](REGLAS.md#ia-de-la-cpu-fase-7) para el detalle de la
heurística (`src/game/ai.c`): qué carta juega la CPU en cada baza (gana con
la más floja que le alcance, o sacrifica la más floja si no puede ganar), y
cómo decide aceptar/escalar envido y truco. Es lógica pura, sin PPU/APU —
testeada en `src/test/test_rules.c` con valores de `entropy` fijos para que
el resultado sea determinístico. La única parte específica de NES es qué le
pasan `canto_ui.c`/`truco_ui.c`/`main.c` como `entropy`:
`nes_frame_count` (`platform/vsync.h`), que varía naturalmente según cuánto
tarda el jugador en decidir cada jugada.

## Interfaz legible y marcador (Fase 8)

La primera versión jugable (Fases 3-7) mostraba las cartas flotando sobre
el paño sin ningún rótulo ni marcador — funcionaba, pero era difícil de
seguir sin conocer el código. La Fase 8 reorganiza toda la pantalla
(`main.c`, constantes `SCORE_ROW`/`CPU_LABEL_ROW`/etc.) para que se
entienda de un vistazo:

```
fila 1:  VOS 00                    CPU 00      <- marcador, siempre visible
fila 2:            CPU                          <- rotula la mano de la CPU
fila 4:  [dorso][dorso][dorso]                   <- mano de la CPU (boca abajo)
filas 8-9:   [carta]      [carta]                <- mesa (lo que se jugo en la baza)
filas 16-19: menu de cantos/truco (envido/flor o truco, segun el momento)
fila 21:           VOS                           <- rotula tu mano
filas 23-24: [carta con marco][carta][carta]     <- tu mano, con borde para
                                                     que se vea como cartas de
                                                     verdad, no numeros sueltos
fila 26:        ^ (cursor, en la carta o subiendo al menu de arriba)
```

(Diagrama de la Fase 8 original; la altura de las cartas y las filas
exactas cambiaron con el rediseño de naipes verticales — ver la sección
más abajo.)

- **Etiquetas "CPU"/"VOS":** con letras ya existentes en la fuente (C,P,U,V,
  O,S), sin necesidad de tiles nuevos.
- **Marco de carta** (`draw_card_border()`/`clear_card_border()` en
  `cards_render.c`): solo alrededor de la mano del jugador (las cartas que
  elige, las que más importa distinguir de un vistazo). Son 8 tiles nuevos
  (esquinas + aristas, tabla de arriba) dibujados en las celdas libres
  alrededor de cada carta — el número/palo de la carta no cambia.
- **Marcador persistente** (`draw_scoreboard()`): "VOS" y "CPU" con su
  puntaje (`draw_number()`, 2 dígitos, sin cero a la izquierda) siempre
  visibles arriba de la pantalla, no solo un flash de color como antes.

### Lección: la fila 0 del nametable cae en el *overscan*

Al verificar este layout en fceux, el marcador (puesto originalmemte en la
fila 0) no aparecía en las capturas de pantalla. La causa: fceux (como la
mayoría de TVs y emuladores) captura/muestra la imagen recortada a 224
líneas visibles en vez de las 240 reales del NTSC/PAL, así que las
primeras ~8 líneas de píxeles (la fila 0 de tiles) y las últimas quedan
ocultas ("overscan"). Cualquier contenido importante tiene que ir al menos
en la fila 1 (`SCORE_ROW` quedó en 1, no en 0). Esto se detectó
comparando, píxel a píxel con Pillow, una captura de fceux contra lo que
se esperaba dibujar — no a simple vista.

## Pulido de interfaz e interacción (después de la Fase 8)

Después de jugarlo a mano, la mayor parte de la confusión no era gráfica
sino de **interacción** e **información faltante**:

- **Bug real (corrupción de pantalla en los menús):** dibujar todas las
  opciones de un menú (hasta ~30 tiles entre texto y cursor) en una sola
  tanda sincronizada a `wait_vblank()` a veces excedía la ventana de
  vblank y corrompía la pantalla (el mismo síntoma que el bug de
  `ppu_finish_vram_update()`, pero por *tamaño* de la tanda en vez de por
  el momento en que se escribe). Se reprodujo instrumentando el código con
  números de depuración en pantalla y comparando capturas con Pillow.
  Arreglo: `choose_option()` (`canto_ui.c`/`truco_ui.c`) y la confirmación
  de carta jugada (`play_trick()` en `main.c`) ahora apagan el render un
  instante para esas tandas grandes, igual que `deal_hand()`.
- **No se entendía qué cantaba la CPU:** ahora `canto_ui.c`/`truco_ui.c`
  muestran "CPU: <opción>" en pantalla un momento cuando la CPU responde o
  escala, en vez de que el jugador solo viera el flash de color final sin
  saber por qué.
- **No se veían las cartas jugadas de una baza a la otra:** en vez de una
  "mesa" compartida que se limpiaba entre bazas, ahora hay una grilla
  persistente de 2 filas x 3 columnas — fila de la CPU arriba, fila del
  jugador abajo, una columna por baza (misma columna que usa esa carta en
  la mano) — que queda visible toda la mano, para poder comparar de un
  vistazo qué carta ganó cada baza.

## Naipes verticales, pantalla de título y cursor unificado (después de la Fase 9)

Tres cambios de interfaz hechos junto con el sonido (Fase 9):

- **Cartas con forma de naipe español:** el esquema de 2 tiles se pasó de
  lado a lado a **apilado** (número arriba, palo abajo, misma columna, ver
  el diagrama al principio de este documento) para que la silueta sea más
  alta que ancha, como un naipe real. El marco de 8 tiles
  (`draw_card_border()`/`clear_card_border()`) se agrandó para envolver esa
  silueta de 1x2, y el dorso de la CPU (`draw_card_back()`) repite el mismo
  tile dos veces verticalmente.
- **Pantalla de título:** `title_screen()` en `main.c` dibuja "TRUCO 86",
  el crédito ("CODE.AR") y los 4 palos, y espera `START` antes de arrancar
  el reparto — antes el juego entraba directo a la primera mano.
- **Cursor de cartas/canto unificado, sin SELECT ni PASO:** el canto de
  truco/retruco/vale cuatro dejó de ser una acción aparte con **SELECT**
  (que quedaba escondida, el jugador no la descubría sola) y pasó a
  integrarse en el mismo menú de opciones que ya usaba envido/flor
  (`canto_ui.c`), navegable desde la fila de cartas: parado en la carta,
  Arriba entra a las opciones de canto (si hay alguna disponible;
  siempre está "Irse al mazo"); Abajo desde la primera opción vuelve a
  elegir carta. Como no hace falta "pasar" nada para simplemente jugar una
  carta, la opción "Paso" desapareció del todo — no cantar es, sencillamente,
  no presionar Arriba. Implementado en `play_trick()` (`main.c`) más
  `truco_list_options()`/`truco_option_text()`/`truco_choose()`
  (`truco_ui.h`/`.c`, que perdieron su antigua `offer_truco()` disparada por
  SELECT).

Estos tres cambios movieron varias filas del layout (cartas más altas,
menú de canto arriba de la mano del jugador, pantalla de título con sus
propias filas); ver las constantes al principio de `main.c`
(`TITLE_*`, `TRUCO_MENU_BASE_ROW`, `CURSOR_ROW`, etc.) para el detalle
actualizado.

## Cartas más grandes, con color por palo

Segunda vuelta de pulido gráfico sobre las cartas de arriba, a pedido de
"que las cartas se entiendan más, más grandes, más parecidas a un naipe
español de verdad":

- **Número de 2 tiles:** el número ocupa 2 tiles de ancho (no 1) usando
  siempre la fuente grande de un solo dígito (`TILE_RANK_1..7`), incluso
  para "10"/"11"/"12" — antes esos tres usaban una fuente chica de 3x5
  apretada en un solo tile, que se veía "chica y rara" comparada con el
  resto. `rank_tiles_for()` en `cards_render.c` devuelve los 2 tiles (`hi`,
  `lo`); para números de un solo dígito reusa los mismos tiles que ya
  existían (`TILE_RANK_1..7`) y para el hueco de la izquierda usa
  `TILE_WHITE` (blanco sólido, tile nuevo) — usar `TILE_BLANK` ahí (como
  hace todavía `draw_number()` para el marcador) deja un cuadrado
  **transparente** que se ve verde (el color de fondo del paño) en vez de
  blanco, porque `TILE_BLANK` es el color 0 (transparente), no blanco.
- **Letra "." (punto):** `TILE_LETTER_DOT`, para que la pantalla de título
  pueda mostrar "CODE.AR" en vez de "CODE AR".
- **Palo de 2x2 tiles (16x16px):** siluetas más grandes y sólidas de
  espada (una hoja con guarda y pomo), basto (un garrote que se ensancha
  hacia abajo), oro (un disco lleno) y copa (un cáliz ancho arriba que se
  angosta en una pata), generadas con `split_quadrants()` en
  `tools/make_chr.py` a partir de un patrón de 16x16 partido en 4 tiles de
  8x8 (`TILE_SUIT_BIG_BASE` en adelante, TL/TR/BL/BR consecutivos por
  palo). Los palos chicos de 8x8 originales se conservan nada más para el
  adorno de la pantalla de título.
- **Un color distinto por palo** (espada azul, basto verde, oro amarillo,
  copa roja — la que ya tenía): la tinta de los tiles de palo sigue siendo
  la misma (`B` para espada/basto, `R` para oro/copa, ver `tools/
  make_chr.py`), lo que cambia es la **paleta de fondo** que la PPU usa
  para ese bloque de pantalla. El NES elige la paleta de a bloques de 2x2
  tiles (no tile por tile) via la tabla de atributos ($23C0-$23FF, 64
  bytes, 2 bits de paleta por cada uno de los 4 cuadrantes de un bloque de
  4x4 tiles) — `ppu_set_palette_block()` (`ppu_draw.c`) mantiene una copia
  en RAM de esa tabla para poder hacer el leer-modificar-escribir sin leer
  la VRAM real, y `draw_card_face()` la llama para las 2 filas del palo
  con la paleta que corresponda (`palette_for_suit()`, `PALETTE_ESPADA`/
  `_BASTO`/`_ORO`/`_DEFAULT` en `cards_render.h`, colores reales en
  `palette[]` en `main.c`). Como el bloque de 2x2 tiles a veces incluye
  también la fila del número o el borde de abajo de la carta (depende de
  en qué fila exacta caiga cada una dentro de la grilla de atributos, no
  hay control mas fino que por bloques de 2x2), esas partes se pintan del
  mismo color que el palo en vez de quedar en blanco/negro — se aceptó ese
  "sangrado" en vez de rehacer todo el layout de filas para que calzara
  perfecto con los bloques de atributos.
- **Bug real encontrado en el camino (corrupción intermitente de la mano
  de la CPU):** después de que `cpu_try_cantar()` corre una negociación de
  canto (mensaje "CPU CANTO X", menú de respuesta), esas funciones dejan
  el render **prendido** al volver (`PPU.mask=0x1E`). La rama de
  `play_trick()` que dibuja la carta de la CPU cuando abre el jugador no
  volvía a apagar el render antes de su propia tanda de escrituras a VRAM
  (`clear_card_back()`/`draw_card_face()`/`draw_card_border()`, ~20
  tiles) — mismo problema de fondo que el bug de pantalla en blanco de
  más arriba (escribir VRAM con el render prendido corrompe escrituras de
  forma intermitente), pero disparado por un camino distinto (una
  negociación de canto en el medio de la baza, no dibujar todo el reparto
  de una). Se reprodujo de forma determinística con capturas de pantalla
  automatizadas por Lua en fceux (leyendo la nametable real, no
  screenshots recortados) y se arregló apagando el render explícitamente
  ahí también, igual que ya hacía la rama simétrica (cuando abre la CPU).

El layout de filas (`CPU_ROW`, `CPU_PLAYED_ROW`, `PLAYER_PLAYED_ROW`,
`TRUCO_MENU_BASE_ROW`, `VOS_LABEL_ROW`, `PLAYER_ROW`, `CURSOR_ROW` en
`main.c`) se recalculó entero para la carta más alta (4 tiles de ancho x 5
de alto con marco, contra 3x4 antes), incluyendo dejarle lugar al peor
caso del menú de responder un canto pendiente (hasta 7 filas: las 4
opciones de truco mezcladas con las 3 de un envido que se puede
interrumpir en el medio, ver `truco_ui.h`).

## Sonido (Fase 9)

`src/platform/sound.c` es un driver mínimo de un solo canal (pulso 1 de la
APU, `$4000-$4003`): cada efecto es un preset de 4 bytes que se escribe de
una sola vez ("tira y olvida"). El *contador de longitud* del hardware
(campo `len_period_high`, bits altos) hace que la nota se corte sola
después de un rato sin que el juego tenga que actualizar nada cuadro a
cuadro — no hay secuenciador ni música, solo SFX cortos:

| Efecto | Cuándo | Tono aproximado |
|---|---|---|
| `SFX_MOVE` | mover el cursor (menú o mano) | agudo, corto |
| `SFX_CONFIRM` | confirmar una opción de menú | medio-agudo, corto |
| `SFX_CARD` | jugar una carta | medio, corto |
| `SFX_CANTO` | la CPU canta/responde/escala | medio-grave, un poco más largo |
| `SFX_WIN` | se ganan puntos (baza, envido/flor o mano) | agudo, largo |
| `SFX_LOSE` | se pierden puntos | grave, largo |

A diferencia de los tiles/VRAM, escribir los registros de la APU **no**
tiene ninguna restricción de vblank (no hay "t register" ni nada
equivalente) — se puede llamar `sound_play()` en cualquier momento del
código, sin apagar el render ni sincronizar con `wait_vblank()`.

## Música de la pantalla de título

`src/platform/music.c` toca una melodía corta en loop por el pulso 2 de la
APU ($4004-$4007), sin tocar el pulso 1 (`sound.c`, los SFX de todo el
juego siguen funcionando igual): `music_init()` prende el canal y arranca
la melodia, `music_update()` hay que llamarla una vez por frame (adentro
del loop de `title_screen()` en `main.c`) para que avance nota por nota, y
`music_stop()` la apaga al presionar START.

El mismo canal también se usa para dos jingles cortos de un solo disparo
(no en loop, ~1.5s): `music_play_win()`/`music_play_lose()`, un arpegio de
La mayor ascendente o descendente (mismas notas que "La Cumparsita" de
abajo, así que no hacen falta periodos nuevos), llamados desde `main.c`
cuando termina de ganarse o perderse una mano (en vez de solo esperar en
silencio con `wait_frames()` como antes).

Es una transcripción nota por nota (y corchea por corchea, todas las notas
son corcheas parejas) de **"La Cumparsita"** (Gerardo Matos Rodríguez,
1916/1917; el compositor murió en 1948, así que está en dominio público
desde hace más de 70 años), tomada de la notación RTTTL del ringtone
clásico de Nokia del tema:

```
8c#1 8c#1 8d1 8c#1 8d1 8c#1 8d1 8c#1 8c#1 8d1 8c#1 8e1 8d1 8c#1
8b0 8a0 8a0 8b0 8c#1 8d1 8c#1 8b0 8a0 8g#0 8a0 8b0 8a0
```

(`8` = corchea, letra = nota, número = octava del formato RTTTL). En
`music.c` esas notas están bajadas una octava (Do#3/Re3/Mi3/Si2/La2/Sol#2
en la numeración de RTTTL, equivalente a Do#4/Re4/Mi4/Si3/La3/Sol#3 en
notación científica) para que el tono sea más cómodo en el canal de pulso
del NES; la duración relativa (todas corcheas iguales) se mantuvo tal
cual. La tabla de notas está en un solo array (`MELODY[]` en `music.c`),
fácil de ajustar si alguien quiere afinarla contra la partitura completa o
reemplazarla por otra pieza de dominio público.

## Verificación sin emulador

`tools/preview_render.py` replica en Python el shuffle del mazo, el mapeo
rank/palo→tile y el layout de `main.c`, y renderiza un PNG a partir de
`res/chr/cards.chr` — útil para revisar el resultado visual rápido durante
el desarrollo. No reemplaza probar el `.nes` real en un emulador con soporte
PAL-N (Mesen), que es la verificación final antes de dar una fase por
cerrada.
