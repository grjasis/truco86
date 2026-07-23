# Reglas de Truco86

Referencia de las reglas tal como se implementan en el motor (`src/game/`).
Sirve como documentación y como fuente de los casos de test en
`src/test/test_rules.c`.

## El mazo

40 cartas españolas (sin 8, 9 ni comodines), 4 palos: espada, basto, oro, copa.
Cada jugador recibe 3 cartas por mano.

## Jerarquía de Truco (de más fuerte a más débil)

| # | Carta(s) | Poder interno |
|---|----------|----------------|
| 1 | 1 de espada ("matador") | 14 |
| 2 | 1 de basto | 13 |
| 3 | 7 de espada | 12 |
| 4 | 7 de oro | 11 |
| 5 | Los cuatro 3 | 10 |
| 6 | Los cuatro 2 | 9 |
| 7 | 1 de oro y 1 de copa ("anchos falsos") | 8 |
| 8 | Los cuatro 12 | 7 |
| 9 | Los cuatro 11 | 6 |
| 10 | Los cuatro 10 | 5 |
| 11 | 7 de basto y 7 de copa ("sietes falsos") | 4 |
| 12 | Los cuatro 6 | 3 |
| 13 | Los cuatro 5 | 2 |
| 14 | Los cuatro 4 | 1 |

Cartas con el mismo poder empatan entre sí ("parda").

## Envido

- Si hay 2 (o 3) cartas del mismo palo en la mano: **20 + suma de las dos
  cartas más altas de ese palo**, contando las figuras (10, 11, 12) como 0.
- Si no hay ningún par del mismo palo: vale la carta suelta de mayor valor
  (As=1 ... Siete=7, figuras=0).
- Cantos: Envido, Envido-Envido (un segundo envido antes de responder),
  Real Envido, Falta Envido. Se cantan antes de jugar la primera carta de la
  mano (`src/platform/canto_ui.c`; la ventana real de truco dura hasta la
  segunda carta, pero acá se simplificó a "antes de la primera" — ver nota
  de implementación más abajo), integrados al mismo cursor que la
  selección de carta de la 1ra baza (`main.c`, `play_trick()`): no hay
  "Paso", si el jugador no quiere cantar elige carta directamente. La CPU
  también puede tomar la iniciativa y cantar ella misma (ver "IA de la
  CPU" más abajo), no solo responder.
- **Valores** (implementados en `src/game/canto.c`, `envido_quiero_value` /
  `envido_no_quiero_value`):

  | Cadena cantada | Quiero | No quiero |
  |---|---|---|
  | Envido | 2 | 1 |
  | Envido, Envido | 4 | 2 |
  | Real Envido (solo) | 3 | 1 |
  | Envido + Real Envido | 5 | 2 |
  | Envido, Envido + Real Envido | 7 | 4 |
  | Falta Envido | puntos que le faltan al que va ganando | igual al "quiero" de la cadena anterior (mínimo 1) |

  Regla general: el valor de "no quiero" de un canto es el valor de "quiero"
  que tenía la cadena *antes* de ese canto (1 si no se había cantado nada
  todavía).
- Empate de puntos de envido: gana quien sea mano esta mano (`hand_mano`
  en `main.c`, rota entre el jugador y la CPU una mano tras otra — ver
  "Partida" más abajo).

## Flor

- Las 3 cartas de la mano son del mismo palo.
- Valor: 20 + suma de los valores de envido de las 3 cartas.
- Con flor no se puede cantar envido (la flor lo reemplaza/prevalece).
- **Valores** (`src/game/canto.c`, `flor_quiero_value` / `flor_no_quiero_value`):

  | Cadena cantada | Quiero | No quiero |
  |---|---|---|
  | Flor | 3 | 1 |
  | Contraflor | 6 | 3 |
  | Contraflor al resto | puntos que le faltan al que va ganando | 6 |

- Si solo un jugador tiene flor, se la lleva automáticamente (3 puntos): no
  hay "quiero/no quiero" posible porque el rival no tiene con qué igualarla.
- **Simplificación actual:** si los dos jugadores tienen flor, `canto_ui.c`
  resuelve directamente comparando los valores de flor al nivel de
  Contraflor (6 puntos), sin menú interactivo de contraflor/contraflor al
  resto (el motor de reglas sí lo soporta — `flor_can_sing_contraflor*` —
  falta la UI en pantalla). Es poco frecuente (los dos jugadores necesitan
  3 cartas del mismo palo cada uno en la misma mano) y queda anotado como
  pendiente.

## Truco

- Escalada: Truco → Retruco → Vale Cuatro. Cada canto se responde con
  Quiero / No Quiero, o escalando al siguiente nivel. Se puede cantar en
  cualquier baza, antes de elegir carta (`src/platform/truco_ui.c`),
  integrado al mismo cursor que la selección de carta (`main.c`).
- **Quién puede cantar el siguiente escalón:** el que cantó el último
  escalón que el rival aceptó (quiero) no puede volver a escalar por su
  cuenta en una baza siguiente — le toca esperar a que el rival (o su IA)
  sea quien suba la apuesta la próxima vez (`truco_last_singer` en
  `main.c`, ver `truco_list_options()`/`truco_cpu_wants_to_initiate()` en
  `truco_ui.c`). Responder a un canto que todavía está "en el aire"
  (sin quiero/no quiero) sí permite escalar de nuevo, sea quien sea.
- **El envido tiene prioridad sobre el truco:** mientras un canto de truco
  esté "en el aire" (todavía sin quiero/no quiero), quien tiene que
  responderlo puede cantar envido en cambio — tanto el jugador respondiendo
  a un truco de la CPU como la CPU respondiendo a un truco del jugador
  (`truco_choose()`/`truco_cpu_initiates()` en `truco_ui.c`, parámetro
  `envido_available`). Una vez que el canto de truco queda aceptado
  (quiero), el envido ya no se puede cantar más esa mano, se haya usado la
  interrupción o no.
- Se juegan 3 bazas (una carta por jugador y ronda); gana la mano quien gane
  2 de 3 bazas (o la primera si hay parda seguida de otra baza ganada, según
  las reglas estándar de desempate de pardas, ver `hand_winner()` en
  `src/game/match.c`). Si la mano ya queda decidida con solo 2 bazas
  jugadas, no se juega la 3ra (`hand_decided_after_two()` en
  `src/game/match.c`).
- **Quién tira primero en cada baza:** en la 1ra baza, el jugador (siempre
  es "mano" hoy, ver la nota sobre rotación de mano en `ARQUITECTURA.md`);
  en las siguientes, quien ganó la baza anterior — si fue parda, sigue
  abriendo el mismo que abrió esa baza (`leader` en `play_hand()`,
  `main.c`). Quien abre no sabe la carta del rival todavía, así que juega
  a ciegas con `ai_choose_lead_card()` (la más floja que le queda, para
  guardarse las fuertes) cuando le toca a la CPU; quien responde ya ve la
  carta del que abrió (`ai_choose_card()`).
- **Valores** (`src/game/canto.c`, `truco_quiero_value` / `truco_no_quiero_value`):

  | Cadena cantada | Quiero | No quiero |
  |---|---|---|
  | (nada, valor por defecto) | 1 | — |
  | Truco | 2 | 1 |
  | Retruco | 3 | 2 |
  | Vale Cuatro | 4 | 3 |

  Misma regla general que envido/flor: el "no quiero" de un canto vale lo
  mismo que la cadena tenía *antes* de ese canto.
- **Irse al mazo:** abandonar la mano en cualquier momento (en vez de
  cantar o de responder a un canto); el rival se lleva lo que valía la mano
  en ese momento (o el valor de "no quiero" del canto pendiente, si había
  uno). Termina la mano inmediatamente, sin jugar las bazas que faltaban.
- **IA de la CPU (Fase 7, `src/game/ai.c`):** responde o escala cuando el
  jugador canta algo, y además puede tomar la iniciativa y cantar ella
  misma (`truco_cpu_wants_to_initiate()` en `src/platform/truco_ui.c`,
  llamado desde `main.c` al principio de cada baza), decidiendo con el
  poder de truco (`card_rank.h`) de su mejor carta sin jugar todavía. Ver
  la sección "IA de la CPU" más abajo.

## IA de la CPU (Fase 7)

Motor puro en `src/game/ai.c`, testeable en el host (`src/test/test_rules.c`):

- **Qué carta jugar** (`ai_choose_card`): la CPU ve la carta que ya jugó el
  jugador (en este juego la CPU siempre responde después). Si tiene alguna
  carta que le gana, juega la ganadora **más floja** que tenga (para
  guardarse las fuertes para las bazas siguientes). Si no puede ganar,
  sacrifica la carta **más floja** que le queda. No tiene en cuenta las
  pardas como caso especial (una carta que empata se trata igual que una
  que pierde).
- **Envido/flor** (`ai_envido_accept`, `ai_envido_escalate`): acepta si su
  propio puntaje de envido supera un umbral (~18-22, con algo de variación,
  ver abajo); escala (canta de nuevo) si supera un umbral más alto
  (~25-29).
- **Truco** (`ai_truco_accept`, `ai_truco_escalate`): igual que envido pero
  usando el poder de truco de su mejor carta sin jugar en vez de puntaje de
  envido (acepta ~7-9, escala ~11-13).
- **Variación (`entropy`):** las cuatro funciones de arriba reciben un byte
  cualquiera que corre el umbral un par de puntos para no ser 100%
  predecibles con la misma mano. En el juego real se les pasa
  `nes_frame_count` (el contador de frames desde `platform/vsync.h`), que
  varía naturalmente según cuánto tarda el jugador en decidir. Los tests de
  host le pasan valores fijos para que el resultado sea determinístico.
- **Iniciativa propia de la CPU** (`truco_cpu_wants_to_initiate()` en
  `truco_ui.c`, `canto_cpu_wants_to_initiate()` en `canto_ui.c`, ambas
  llamadas desde `cpu_try_cantar()` en `main.c`): la CPU puede cantar
  truco/retruco/vale cuatro por su cuenta reusando el mismo umbral que
  `ai_truco_escalate` (mano lo bastante fuerte para escalar), y puede
  abrir el envido con el mismo criterio que `ai_envido_accept`/
  `ai_envido_escalate` (envido u opcionalmente real envido directo si el
  puntaje es muy alto) — ya no solo responde cuando el jugador canta
  primero. Sigue sin poder irse al mazo por su cuenta (eso queda para una
  IA más completa).
- **Cuándo le toca hablar a la CPU:** solo en su propio turno, nunca antes
  de que hable quien abre la baza (`leader`, ver más abajo) — si abre la
  CPU, antes de elegir su carta de apertura; si abre el jugador, recién
  después de que el jugador ya jugó su carta sin cantar nada (justo antes
  de que la CPU tenga que responder con una carta). Antes, la CPU
  intentaba cantar apenas empezaba la baza sin importar quién abría, lo
  que la hacía "adelantarse" al jugador cuando era su turno de hablar.

## Partida (Fase 8)

- **Pantalla de inicio:** antes de la primera mano, un menú de 4 opciones
  (`title_screen()` en `src/main.c`) para elegir con qué reglas se juega
  esta partida: **CON FLOR A 15** / **CON FLOR A 30** / **SIN FLOR A 15** /
  **SIN FLOR A 30**. "Con/sin flor" habilita o no la resolución automática
  de flor (`flor_enabled`, ver la sección "Flor" más arriba: con flor
  deshabilitada, una mano de 3 cartas del mismo palo no hace nada especial,
  se ofrece envido normal); "a 15/30" fija `target_score` (cuántos puntos
  hace falta para ganar la partida).
- Se juega a `target_score` puntos (elegido en la pantalla de inicio, ver
  arriba). Se juegan manos una atrás de otra (`play_hand()` en un loop
  dentro de `main()`) hasta que alguno de los dos llega al objetivo.
- **Mano rotativa:** quién es "mano" (habla/tira primero en la 1ra baza de
  la mano, y gana los empates de envido/flor) rota entre el jugador y la
  CPU una mano tras otra (`hand_mano` en `main.c`: arranca en el jugador
  al empezar la partida, y se invierte después de cada `play_hand()`, sin
  importar quién ganó). Dentro de la misma mano, quién tira primero en las
  bazas 2 y 3 sigue dependiendo de quién ganó la baza anterior (ver
  "Truco" más arriba), no de la mano.
- **"Falta envido"/"contraflor al resto":** valen los puntos que le faltan
  al que va ganando la partida para llegar a `target_score` (`falta_target()`
  en `main.c`, pasado a `canto_choose()`/`canto_cpu_initiates()`/
  `canto_try_flor()` en `canto_ui.c`) — no un valor fijo, así que cambia
  según el marcador y según si se está jugando a 15 o a 30.
- El marcador (`VOS` / `CPU`) se ve todo el tiempo arriba de la pantalla y
  se actualiza apenas se ganan puntos (de envido/flor o de una mano), en
  vez de solo mostrarse con un flash de color como en fases anteriores.
- Cuando alguien llega al objetivo, se limpia la pantalla y se muestra
  "GANASTE" o "PERDISTE" junto con el marcador final.
- **No implementado todavía:** la distinción "malas"/"buenas" (mitad de
  partida) no tiene ningún efecto visual ni de reglas — es solo el conteo
  simple hasta el objetivo.

## Notas de implementación

- La carta se representa como un byte `0..39`: `suit = carta / 10`,
  `rank = carta % 10` (rank 0..6 = As..Siete, rank 7..9 = 10,11,12).
- Todo el motor de reglas (`src/game/`) es C portable sin dependencias de
  hardware, para poder testearlo en el host con `make test` sin necesidad de
  un emulador de NES.
- **Falta Envido / Contraflor al resto sin tanteador todavía:** como
  todavía no existe un tanteador de partida real (a 15/30), el valor
  de "cuánto le falta al que va ganando" usa un placeholder fijo
  (`FALTA_TARGET_PLACEHOLDER` en `src/platform/canto_ui.c`). Cuando exista
  el tanteador, ese valor se calcula de verdad y se lo pasa a
  `envido_quiero_value`/`flor_quiero_value`, que ya están preparadas para
  recibirlo como parámetro.
- **Ventana de cantos simplificada:** la regla real de truco permite cantar
  envido/flor hasta que se juega la segunda carta de la mano. Esta
  implementación simplifica eso a "se canta todo antes de jugar la primera
  carta" (`run_canto_phase()` corre entera antes de `play_trick()`).
