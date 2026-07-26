/*
 * Truco86 - juego de Truco para NES (PAL-N)
 * Creditos: grjasis@code.ar
 *
 * main.c - Fases 4 a 9: pantalla de titulo (con un menu para elegir
 * con/sin flor y a 15 o 30 puntos, ver title_screen()), y despues juega
 * manos de Truco una detras de otra, sumando puntos en un marcador
 * persistente ("VOS"/"CPU" arriba de todo), hasta que alguno llega a
 * target_score. En cada mano: se reparte, y si alguien tiene flor (y esta
 * habilitada) se resuelve sola (sin menu, platform/canto_ui.c); si no, se
 * juegan hasta 3 bazas con un cursor
 * combinado (ver play_trick()): abajo se elige carta con Izquierda/
 * Derecha, y con Arriba se sube a un menu de opciones de canto — truco/
 * retruco/vale cuatro/irse al mazo siempre (platform/truco_ui.c), y en la
 * 1ra baza tambien envido/real/falta si nadie canto flor (platform/
 * canto_ui.c) — no hace falta "pasar" ningun menu para jugar una carta ni
 * para saltear el envido. La CPU juega su carta y responde/escala los
 * cantos con la IA de game/ai.c (Fase 7). Si alguien ya se llevo 2 de las
 * primeras 2 bazas (o la 1ra fue parda y la 2da no), la mano termina ahi,
 * sin jugar la 3ra (game/match.c, hand_decided_after_two()). Las cartas
 * son verticales (como un naipe de verdad) y llevan marco (Fase 8/pulido).
 * Hay SFX cortos para cursor/confirmar/jugar carta/cantar/ganar-perder
 * (platform/sound.c, Fase 9), sin musica.
 */

#include <nes.h>
#include <joystick.h>
#include "game/deck.h"
#include "game/card_rank.h"
#include "game/match.h"
#include "game/canto.h"
#include "game/envido.h"
#include "game/ai.h"
#include "platform/ppu_draw.h"
#include "platform/cards_render.h"
#include "platform/vsync.h"
#include "platform/input.h"
#include "platform/text.h"
#include "platform/canto_ui.h"
#include "platform/truco_ui.h"
#include "platform/sound.h"
#include "platform/music.h"

/* 4 paletas de fondo (PALETTE_DEFAULT/ESPADA/BASTO/ORO, ver
   cards_render.h): una por palo, para que cada palo se dibuje de un color
   distinto (draw_card_face() en cards_render.c elige cual con
   ppu_set_palette_block()). El indice 0 (verde paño) es el color
   "universal de fondo" del hardware: las 4 paletas de fondo comparten
   fisicamente esa entrada ($3F00 se refleja en $3F04/$3F08/$3F0C), asi que
   tiene que ser el mismo valor en las 4. Lo mismo con el color 2 (negro): es
   el que usan el numero de la carta y su marco, y como el bloque de atributos
   que colorea el palo tambien los alcanza, tiene que quedar negro en las 4
   para que el numero se lea igual sea cual sea el palo. El unico color que
   cambia entre paletas es el 3, la tinta del palo. */
static const unsigned char palette[32] = {
    0x1A, 0x30, 0x0F, 0x16,   /* PALETTE_DEFAULT: verde paño / blanco / negro / rojo (texto, bordes, dorso, copa) */
    0x1A, 0x30, 0x0F, 0x11,   /* PALETTE_ESPADA: azul */
    0x1A, 0x30, 0x0F, 0x2A,   /* PALETTE_BASTO: verde (mas vivo que el paño) */
    0x1A, 0x30, 0x0F, 0x28,   /* PALETTE_ORO: amarillo */
    0x1A, 0x30, 0x0F, 0x16,   /* sprites (sin usar todavia) */
    0x1A, 0x30, 0x0F, 0x16,
    0x1A, 0x30, 0x0F, 0x16,
    0x1A, 0x30, 0x0F, 0x16
};

/* Columna de cada una de las 3 cartas de una mano (col del contenido, no
   del marco: cada carta ocupa col/col+1). Las cartas son verticales
   (numero arriba, palo abajo, 2 tiles de ancho + marco), asi que con
   separar cada slot 8 columnas alcanza de sobra para el marco (4 tiles de
   ancho) sin que se toquen entre si.

   Las columnas tienen que ser PARES: draw_card_face() elige la paleta del
   palo con ppu_set_palette_block(), y la tabla de atributos de la PPU agrupa
   de a 2x2 tiles alineados a columnas pares — si la carta arrancara en una
   columna impar, sus 2 columnas caerian en bloques distintos y solo se
   pintaria de color la mitad del palo. 6/14/22 es el juego de columnas pares
   que deja la fila de cartas mas centrada (ocupa las columnas 5 a 24 de 32
   contando los marcos). */
static const unsigned char HAND_COL[3] = { 6, 14, 22 };

/* --- Layout de pantalla (32 columnas x 30 filas) ---
   Cada carta boca arriba ocupa 2 tiles de ancho x 3 de alto de contenido
   (numero grande de 2 tiles arriba, palo grande de 2x2 tiles abajo) + el
   marco (1 fila arriba, 1 fila abajo, 1 columna a cada lado) = 4 columnas
   x 5 filas en total; ver draw_card_face()/draw_card_border() en
   cards_render.c. Las cartas boca abajo (dorso, mano de la CPU) son mas
   chicas (2x2, sin marco: no hace falta mostrar numero/palo tapado).

   Barra de estado (filas 1 y 2) --- Fila 1: marcador de los dos, y en el medio cuanto vale la mano
   ("VALE n", que sube con truco/retruco/vale cuatro). Fila 2: una linea
   separadora de punta a punta que separa la barra de la mesa, con la palabra
   "MANO" incrustada del lado del que es mano esta mano. */
#define SCORE_ROW           1 /* fila 0 cae en el overscan, no se ve en TVs/la mayoria de emuladores */
#define SCORE_VOS_LABEL_COL 2
#define SCORE_VOS_NUM_COL   6
#define SCORE_CPU_LABEL_COL 21
#define SCORE_CPU_NUM_COL   25
#define STAKE_COL           13 /* "VALE" */
#define STAKE_DIGIT_COL     18 /* el numero (1..4), siempre de un solo digito */

#define RULE_ROW      2
#define RULE_COL      1
#define RULE_LEN      30
#define MANO_VOS_COL  2  /* "MANO" incrustado en la linea, alineado con "VOS" */
#define MANO_CPU_COL 21  /* idem, alineado con "CPU" */

/* Mano de la CPU (dorsos de 2x2, sin marco), se va vaciando. No lleva
   etiqueta "CPU" al lado: ya esta en la barra de estado, y tres palabras
   apiladas contra el margen izquierdo (VOS / MANO / CPU) amontonaban esa
   esquina. */
#define CPU_ROW       3

/* Cartas ya jugadas, una columna por baza (misma columna que HAND_COL, asi
   se puede comparar de un vistazo que carta gano cada baza): fila de la
   CPU arriba, fila del jugador debajo, en la MISMA columna. Con marco.
   Quedan visibles toda la mano (no se borran hasta la proxima). */
#define CPU_PLAYED_ROW    6  /* marco: fila 5 y 9 */
#define PLAYER_PLAYED_ROW 11 /* marco: fila 10 y 14 */

/* Marca de quien gano cada baza (triangulo hacia el ganador, "=" si fue
   parda), en el margen que queda a la izquierda de cada columna de baza y a
   la altura donde se tocan la carta de la CPU y la del jugador. Asi se ve de
   un vistazo como va la mano (1-0, 1-1, ...) sin depender del flash de color,
   que pasa y no queda. */
#define MARK_ROW      10
#define MARK_COL_OFF   3 /* columnas a la izquierda de HAND_COL[baza] */

/* Linea de mesa: una raya punteada en MARK_ROW que separa la mitad de la CPU
   de la del jugador, dibujada solo en los huecos que dejan las 3 columnas de
   cartas (las cartas del jugador ocupan ahi su fila de marco). Ademas de
   ordenar la pantalla, llena el vacio verde que queda al principio de la mano
   cuando todavia no se jugo ninguna carta. Cada tramo es {columna, largo}. */
static const unsigned char TABLE_LINE[4][2] = { {1,4}, {9,4}, {17,4}, {25,6} };

/* El mensaje "CPU: ..." usa la fila 15; el menu de respuesta a un canto
   pendiente (truco_ui.c) y el de envido/flor (canto_ui.c) las filas
   16-22 (hasta 7 opciones en el peor caso: truco puede mezclar sus 4 con
   las 3 de un envido que se puede interrumpir en el medio, ver
   truco_ui.h). El menu de cantar truco/envido por primera vez esta
   INTEGRADO con la seleccion de carta (ver play_trick()): las opciones se
   dibujan en esas mismas filas, pegadas arriba de la mano, empezando por
   la fila mas cercana a las cartas (TRUCO_MENU_BASE_ROW) y subiendo. */
#define MENU_COL 11
#define MENU_CURSOR_COL 9
#define TRUCO_MENU_BASE_ROW 22

#define PLAYER_ROW    24 /* marco: fila 23 y 27 */
#define CURSOR_ROW    28 /* ultima fila visible (la 29 cae en el overscan, igual que la 0) */

/* Pantalla final: todo adentro de un marco centrado. */
#define RESULT_FRAME_COL  6
#define RESULT_FRAME_ROW  9
#define RESULT_FRAME_W   20
#define RESULT_FRAME_H   11
#define RESULT_ROW       12
#define RESULT_COL       12 /* "GANASTE" (7) queda centrado; "PERDISTE" (8) se dibuja una columna a la izquierda */
#define RESULT_SCORE_ROW 16
#define RESULT_VOS_COL    9 /* "VOS nn" + "CPU nn" quedan centrados en el marco (columnas 6..25) */
#define RESULT_CPU_COL   17

#define TITLE_FRAME_COL  3
#define TITLE_FRAME_ROW  3
#define TITLE_FRAME_W   26
#define TITLE_FRAME_H   22

#define TITLE_ROW      6
#define TITLE_COL      12
#define TITLE_CREDIT_ROW 13
#define TITLE_CREDIT_COL 12
/* Los 4 palos GRANDES (16x16 = 2x2 tiles cada uno) como adorno. Columnas
   pares por la misma razon que HAND_COL: cada palo se pinta de su color con
   ppu_set_palette_block(). */
#define TITLE_SUITS_ROW  9
#define TITLE_SUITS_COL  10
#define TITLE_SUITS_STEP  4

/* Menu de la pantalla de titulo: elegir con/sin flor y a 15 o 30 puntos
   (ver title_screen()), mismo patron de cursor que los demas menus del
   juego (MENU_CURSOR_COL/columna de texto separadas). */
#define TITLE_MENU_CURSOR_COL 7
#define TITLE_MENU_COL         9
#define TITLE_MENU_ROW        16
#define TITLE_MENU_STEP        2 /* una fila vacia entre opciones: pegadas se leian apelmazadas */
#define NUM_START_OPTIONS      4

#define BACKDROP_NEUTRAL 0x1A /* verde pano */
#define BACKDROP_WIN     0x22 /* azul: gana el jugador */
#define BACKDROP_LOSE    0x16 /* rojo: gana la CPU */
#define BACKDROP_TIE     0x00 /* gris: mano parda */

#define WAIT_AFTER_TRICK_FRAMES 75 /* ~1.5s a 50 cuadros/seg (PAL-N) */

static Card player_hand[3];
static Card cpu_hand[3];
static unsigned char player_played[3];
static unsigned char cpu_played[3];
static unsigned char results[3];
static TrucoChain truco_chain;
static unsigned char truco_last_singer; /* TRUCO_SINGER_*, ver truco_ui.h */
static unsigned char player_cursor;
static unsigned char score_player;
static unsigned char score_cpu;
static unsigned char target_score;  /* 15 o 30, elegido en title_screen() */
static unsigned char flor_enabled;  /* 1 = con flor, 0 = sin flor, elegido en title_screen() */
static unsigned char hand_mano;     /* TRICK_PLAYER o TRICK_CPU: quien es mano esta mano (habla/tira primero), rota en main() despues de cada mano */
static unsigned int shuffle_seed = 20260722u;

/* Cuanto le falta al que va ganando la partida para llegar a target_score
   (para "falta envido"/"contraflor al resto", ver game/canto.c). */
static unsigned char falta_target(void)
{
    unsigned char leader_score = (score_player > score_cpu) ? score_player : score_cpu;
    return (unsigned char)(target_score - leader_score);
}

static unsigned char find_unplayed(const unsigned char played[3], unsigned char from, signed char dir)
{
    unsigned char idx = from;
    unsigned char i;
    for (i = 0; i < 3; ++i) {
        idx = (unsigned char)((idx + dir + 3) % 3);
        if (!played[idx]) return idx;
    }
    return from;
}

static void wait_frames(unsigned char n)
{
    unsigned char i;
    for (i = 0; i < n; ++i) {
        wait_vblank();
    }
}

/* Poder de truco (ver card_rank.h) de la mejor carta que le queda a la CPU
   sin jugar, para la heuristica de respuesta de truco_ui.c. */
static unsigned char cpu_best_remaining_power(void)
{
    unsigned char best = 0;
    unsigned char i;
    for (i = 0; i < 3; ++i) {
        if (!cpu_played[i]) {
            unsigned char p = truco_power(cpu_hand[i]);
            if (p > best) best = p;
        }
    }
    return best;
}

static void draw_scoreboard(void)
{
    draw_text(SCORE_VOS_LABEL_COL, SCORE_ROW, "VOS");
    draw_number(SCORE_VOS_NUM_COL, SCORE_ROW, score_player);
    draw_text(SCORE_CPU_LABEL_COL, SCORE_ROW, "CPU");
    draw_number(SCORE_CPU_NUM_COL, SCORE_ROW, score_cpu);
}

/* Cuanto vale la mano ahora mismo (1 sin truco, 2/3/4 con truco/retruco/vale
   cuatro), en el medio de la barra: es el dato que mas cambia la decision de
   jugar y antes no se mostraba en ningun lado. */
static void draw_stake(void)
{
    draw_text(STAKE_COL, SCORE_ROW, "VALE");
    ppu_set_tile(STAKE_DIGIT_COL, SCORE_ROW, digit_tile(truco_hand_value(&truco_chain)));
}

/* Linea separadora entre la barra y la mesa, con "MANO" incrustado del lado
   del que es mano esta mano (rota cada mano, ver main()). */
static void draw_mano_rule(void)
{
    unsigned char col = (unsigned char)((hand_mano == TRICK_PLAYER) ? MANO_VOS_COL : MANO_CPU_COL);
    draw_rule(RULE_COL, RULE_ROW, RULE_LEN);
    ppu_set_tile((unsigned char)(col - 1), RULE_ROW, TILE_BLANK);
    draw_text(col, RULE_ROW, "MANO");
    ppu_set_tile((unsigned char)(col + 4), RULE_ROW, TILE_BLANK);
}

/* Redibuja solo los dos numeros del marcador con el render prendido (justo
   despues de un wait_vblank(), como toda escritura de VRAM durante el juego).
   Son 4 tiles: entra de sobra en una ventana de vblank. El resto de la barra
   (etiquetas, linea, "MANO") se dibuja una sola vez por mano en deal_hand(),
   con el render apagado. */
static void refresh_scoreboard(void)
{
    wait_vblank();
    draw_number(SCORE_VOS_NUM_COL, SCORE_ROW, score_player);
    draw_number(SCORE_CPU_NUM_COL, SCORE_ROW, score_cpu);
    ppu_finish_vram_update(0x80);
}

/* Idem para el "VALE n", despues de que una negociacion de truco cambie lo
   que vale la mano. */
static void refresh_stake(void)
{
    wait_vblank();
    ppu_set_tile(STAKE_DIGIT_COL, SCORE_ROW, digit_tile(truco_hand_value(&truco_chain)));
    ppu_finish_vram_update(0x80);
}

/* Marca el resultado de una baza ya jugada en el margen de su columna. */
static void draw_trick_mark(unsigned char trick, unsigned char result)
{
    unsigned char tile = (result == TRICK_PLAYER) ? TILE_MARK_PLAYER
                       : (result == TRICK_CPU)    ? TILE_MARK_CPU
                                                  : TILE_MARK_TIE;
    wait_vblank();
    ppu_set_tile((unsigned char)(HAND_COL[trick] - MARK_COL_OFF), MARK_ROW, tile);
    ppu_finish_vram_update(0x80);
}

/* Puntero ancho (2 tiles) debajo de la carta 'idx' de la mano del jugador. */
static void set_card_cursor(unsigned char idx, unsigned char show)
{
    unsigned char col = HAND_COL[idx];
    ppu_set_tile(col, CURSOR_ROW, show ? TILE_CURSOR_WIDE_L : TILE_BLANK);
    ppu_set_tile((unsigned char)(col + 1), CURSOR_ROW, show ? TILE_CURSOR_WIDE_R : TILE_BLANK);
}

static void apply_points(signed char delta)
{
    if (delta > 0) {
        score_player = (unsigned char)(score_player + delta);
    } else if (delta < 0) {
        score_cpu = (unsigned char)(score_cpu - delta);
    }
}

/* Cambia el fondo un rato para resaltar quien gano puntos y despues lo
   vuelve al color neutro. El marcador numerico (arriba de todo) es la
   fuente de verdad; esto es solo un resalte visual adicional. */
static void flash_backdrop(unsigned char color, unsigned char frames)
{
    wait_vblank();
    ppu_set_backdrop(color);
    ppu_finish_vram_update(0x80);
    wait_frames(frames);
    wait_vblank();
    ppu_set_backdrop(BACKDROP_NEUTRAL);
    ppu_finish_vram_update(0x80);
}

/* Suma/resta puntos y lo muestra (marcador + flash de color + SFX). Se le
   pasa como callback a truco_choose()/truco_cpu_initiates() (ver
   truco_ui.h) para el envido que se puede cantar en el medio de una
   negociacion de truco todavia sin responder (prioridad del envido). */
static void award_points_and_flash(signed char delta)
{
    apply_points(delta);
    refresh_scoreboard();
    sound_play((delta > 0) ? SFX_WIN : SFX_LOSE);
    flash_backdrop((delta > 0) ? BACKDROP_WIN : BACKDROP_LOSE, WAIT_AFTER_TRICK_FRAMES);
}

/* Reparte una mano nueva: apaga el render para poder limpiar y redibujar
   toda la pantalla de una (reparto, marcador, etiquetas), y lo vuelve a
   prender al final. */
static void deal_hand(void)
{
    Card deck[DECK_SIZE];
    unsigned char i;

    PPU.mask = 0x00; /* render apagado: momento seguro para escribir VRAM sin limite */

    ppu_clear_nametable0();
    ppu_clear_oam();

    /* semilla distinta cada mano: se mezcla con nes_frame_count (cuanto
       tardo el jugador en decidir la mano anterior), asi no se repite
       siempre el mismo reparto. */
    shuffle_seed = (unsigned int)(shuffle_seed * 31u + nes_frame_count + 1u);
    deck_init_shuffled(deck, shuffle_seed);

    truco_chain.step = TRUCO_STEP_NONE;
    truco_last_singer = TRUCO_SINGER_NONE;
    player_cursor = 0;

    /* barra de estado completa (marcador + "VALE n" + linea con "MANO"):
       se dibuja entera una sola vez por mano, aca, con el render apagado.
       draw_stake() necesita truco_chain ya reseteado. */
    draw_scoreboard();
    draw_stake();
    draw_mano_rule();
    for (i = 0; i < 4; ++i) {
        draw_rule(TABLE_LINE[i][0], MARK_ROW, TABLE_LINE[i][1]);
    }

    for (i = 0; i < 3; ++i) {
        player_hand[i] = deck[i];
        cpu_hand[i] = deck[3 + i];
        player_played[i] = 0;
        cpu_played[i] = 0;

        draw_card_face(HAND_COL[i], PLAYER_ROW, player_hand[i]);
        draw_card_border(HAND_COL[i], PLAYER_ROW);
        draw_card_back(HAND_COL[i], CPU_ROW);
    }

    ppu_reset_scroll();
    PPU.control = 0x80; /* NMI on */
    PPU.mask = 0x1E;    /* fondo + sprites visibles */
}

/* El cursor combinado puede tener, ademas de las opciones de truco,
   opciones de envido/flor (solo en la 1ra baza, y solo mientras el
   jugador no las haya resuelto todavia): 'kind' distingue de cual de los
   dos menus (KIND_TRUCO/KIND_ENVIDO) es cada entrada de la lista
   combinada, para poder pedirle el texto y, al elegirla, la negociacion
   correcta a truco_ui.c o canto_ui.c. */
#define KIND_TRUCO  0
#define KIND_ENVIDO 1

static const char *combined_option_text(unsigned char kind, unsigned char code)
{
    return (kind == KIND_TRUCO) ? truco_option_text(code) : canto_option_text(code);
}

/* Dibuja (o borra, si show==0) las opciones combinadas de canto pegadas
   arriba de la mano, de abajo (base, la mas cercana a las cartas) hacia
   arriba: primero las de truco (indices 0..truco_count-1), despues arriba
   de esas las de envido (si las hay). Es una tanda que puede ser grande
   (hasta 6 lineas), asi que se hace con el render apagado (misma razon
   que canto_ui.c). */
static void draw_canto_menu(const unsigned char *kind, const unsigned char *code, unsigned char count, unsigned char show)
{
    unsigned char i;
    PPU.mask = 0x00;
    for (i = 0; i < count; ++i) {
        unsigned char row = (unsigned char)(TRUCO_MENU_BASE_ROW - i);
        /* siempre se borra primero la columna del cursor en esta fila,
           haya o no opcion (por si quedo algo de una tanda anterior, de
           una escalada previa en esta misma baza): asi el unico lugar
           donde puede haber un cursor dibujado es el que pone el propio
           codigo de navegacion, nunca un resto. */
        ppu_set_tile(MENU_CURSOR_COL, row, TILE_BLANK);
        if (show) {
            draw_text(MENU_COL, row, combined_option_text(kind[i], code[i]));
        } else {
            clear_text(MENU_COL, row, 7);
        }
    }
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;
}

/* Arma la lista combinada (truco, y en la 1ra baza tambien envido si
   'offer_envido') en 'kind'/'code'. Devuelve la cantidad total. */
static unsigned char build_canto_menu(unsigned char offer_envido, unsigned char *kind, unsigned char *code)
{
    unsigned char can_escalate = (unsigned char)(truco_last_singer != TRUCO_SINGER_PLAYER);
    unsigned char truco_n = truco_list_options(&truco_chain, can_escalate, code);
    unsigned char i;
    unsigned char n = truco_n;

    for (i = 0; i < truco_n; ++i) kind[i] = KIND_TRUCO;

    if (offer_envido) {
        unsigned char envido_opts[CANTO_MAX_OPTIONS];
        unsigned char envido_n = canto_list_options(envido_opts);
        for (i = 0; i < envido_n; ++i) {
            kind[n] = KIND_ENVIDO;
            code[n] = envido_opts[i];
            ++n;
        }
    }
    return n;
}

/* Le da a la CPU la oportunidad de cantar ella misma (envido y/o truco,
   ver ai.h) ANTES de jugar una carta. Se llama solo cuando le toca hablar
   a la CPU (ver play_trick(): si abre ella, antes de elegir su carta de
   apertura; si abre el jugador, recien despues de que el jugador ya
   decidio "solo" jugar carta sin cantar nada, justo antes de que la CPU
   tenga que responder con una carta) — nunca antes de que hable quien
   abre la baza, para no "saltarse" el turno del otro (regla real: se
   puede cantar antes de jugar la propia carta, no antes de la del
   rival). Devuelve 1 si la mano termino por un canto (ya deja *hand_ended
   / *early_points listos), 0 si hay que seguir jugando la baza. */
static unsigned char cpu_try_cantar(unsigned char *offer_envido, unsigned char *hand_ended, signed char *early_points)
{
    if (*offer_envido) {
        unsigned char envido_code;
        if (canto_cpu_wants_to_initiate(envido_score(cpu_hand), nes_frame_count, &envido_code)) {
            signed char envido_delta = canto_cpu_initiates(envido_code, player_hand, cpu_hand, falta_target(),
                                                            (unsigned char)(hand_mano == TRICK_PLAYER));
            if (envido_delta != 0) award_points_and_flash(envido_delta);
            *offer_envido = 0;
        }
    }
    {
        unsigned char truco_code;
        unsigned char can_initiate = (unsigned char)(truco_last_singer != TRUCO_SINGER_CPU);
        if (truco_cpu_wants_to_initiate(&truco_chain, cpu_best_remaining_power(), nes_frame_count, can_initiate, &truco_code)) {
            signed char truco_delta = truco_cpu_initiates(&truco_chain, truco_code, cpu_best_remaining_power(),
                                                           &truco_last_singer, offer_envido, player_hand, cpu_hand,
                                                           award_points_and_flash, falta_target(),
                                                           (unsigned char)(hand_mano == TRICK_PLAYER));
            refresh_stake(); /* el truco aceptado cambio cuanto vale la mano */
            /* una vez que se negocia un canto de truco (aceptado, o la
               mano termina), el envido ya no se puede cantar mas, se haya
               usado o no la interrupcion de arriba (ver truco_ui.h). */
            *offer_envido = 0;
            if (truco_delta != 0) {
                *hand_ended = 1;
                *early_points = truco_delta;
                return 1;
            }
        }
    }
    return 0;
}

/* Un solo cursor combinado: abajo del todo se elige carta (Izquierda/
   Derecha), y presionando Arriba desde la primera carta se sube a las
   opciones de canto (truco/retruco/vale cuatro/irse al mazo siempre; en la
   1ra baza tambien envido/real/falta si nadie tiene flor), navegables con
   Arriba/Abajo; Abajo desde la primera opcion vuelve a las cartas. No hace
   falta "Paso" para nada: si el jugador no quiere cantar, elige carta
   directamente.

   'leader' (TRICK_PLAYER o TRICK_CPU, ver play_hand()) es quien tira
   primero esta baza: la regla real es que abre quien gano la baza
   anterior (parda: sigue abriendo el mismo). Si abre la CPU, su carta se
   elige y se muestra ANTES de que el jugador elija la suya (con
   ai_choose_lead_card(), sin saber todavia la carta del jugador); si abre
   el jugador (el caso de siempre hasta ahora), la CPU responde al final
   con ai_choose_card() ya sabiendo que jugo el jugador. */
static unsigned char play_trick(unsigned char trick, unsigned char offer_envido, unsigned char leader, unsigned char *hand_ended, signed char *early_points)
{
    unsigned char cpu_idx;
    Card player_card;
    Card cpu_card;
    unsigned char pressed;
    unsigned char canto_kind[TRUCO_MAX_OPTIONS + CANTO_MAX_OPTIONS];
    unsigned char canto_code[TRUCO_MAX_OPTIONS + CANTO_MAX_OPTIONS];
    unsigned char canto_count;
    unsigned char in_menu;   /* 0 = eligiendo carta, 1 = en las opciones de canto */
    unsigned char menu_sel;

    /* Si abre la CPU (gano la baza anterior), su carta se decide y se
       muestra ya (fila CPU_PLAYED_ROW de esta baza) antes de que el
       jugador elija: asi el jugador ve con que abrio antes de responder,
       como en una partida real. Antes de eso, es su momento de cantar si
       quiere (ver cpu_try_cantar()). */
    if (leader == TRICK_CPU) {
        if (cpu_try_cantar(&offer_envido, hand_ended, early_points)) {
            return TRICK_TIE; /* no se usa: la mano termino por un canto, no por la baza */
        }
        cpu_idx = ai_choose_lead_card(cpu_hand, cpu_played);
        cpu_card = cpu_hand[cpu_idx];
        cpu_played[cpu_idx] = 1;

        PPU.mask = 0x00;
        clear_card_back(HAND_COL[cpu_idx], CPU_ROW);
        draw_card_face(HAND_COL[trick], CPU_PLAYED_ROW, cpu_card);
        draw_card_border(HAND_COL[trick], CPU_PLAYED_ROW);
        ppu_reset_scroll();
        PPU.control = 0x80;
        PPU.mask = 0x1E;
    }

    canto_count = build_canto_menu(offer_envido, canto_kind, canto_code);

    player_cursor = find_unplayed(player_played, player_cursor, 0);
    if (player_played[player_cursor]) {
        player_cursor = find_unplayed(player_played, player_cursor, 1);
    }

    in_menu = 0;
    menu_sel = 0;

    draw_canto_menu(canto_kind, canto_code, canto_count, 1);
    wait_vblank();
    set_card_cursor(player_cursor, 1);
    ppu_finish_vram_update(0x80);

    for (;;) {
        wait_vblank();
        input_update();
        pressed = input_pressed();

        if (!in_menu) {
            if (pressed & JOY_LEFT_MASK) {
                set_card_cursor(player_cursor, 0);
                player_cursor = find_unplayed(player_played, player_cursor, -1);
                set_card_cursor(player_cursor, 1);
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if (pressed & JOY_RIGHT_MASK) {
                set_card_cursor(player_cursor, 0);
                player_cursor = find_unplayed(player_played, player_cursor, 1);
                set_card_cursor(player_cursor, 1);
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if ((pressed & JOY_UP_MASK) && canto_count > 0) {
                in_menu = 1;
                menu_sel = 0;
                set_card_cursor(player_cursor, 0);
                ppu_set_tile(MENU_CURSOR_COL, TRUCO_MENU_BASE_ROW, TILE_ARROW_RIGHT);
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if (pressed & JOY_BTN_A_MASK) {
                sound_play(SFX_CARD);
                break;
            }
        } else {
            if (pressed & JOY_DOWN_MASK) {
                if (menu_sel == 0) {
                    in_menu = 0;
                    ppu_set_tile(MENU_CURSOR_COL, TRUCO_MENU_BASE_ROW, TILE_BLANK);
                    set_card_cursor(player_cursor, 1);
                } else {
                    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(TRUCO_MENU_BASE_ROW - menu_sel), TILE_BLANK);
                    --menu_sel;
                    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(TRUCO_MENU_BASE_ROW - menu_sel), TILE_ARROW_RIGHT);
                }
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if (pressed & JOY_UP_MASK) {
                if (menu_sel < (unsigned char)(canto_count - 1)) {
                    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(TRUCO_MENU_BASE_ROW - menu_sel), TILE_BLANK);
                    ++menu_sel;
                    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(TRUCO_MENU_BASE_ROW - menu_sel), TILE_ARROW_RIGHT);
                    ppu_finish_vram_update(0x80);
                    sound_play(SFX_MOVE);
                }
            } else if (pressed & JOY_BTN_A_MASK) {
                unsigned char chosen_kind = canto_kind[menu_sel];
                unsigned char chosen_code = canto_code[menu_sel];

                ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(TRUCO_MENU_BASE_ROW - menu_sel), TILE_BLANK);
                ppu_finish_vram_update(0x80);
                draw_canto_menu(canto_kind, canto_code, canto_count, 0);
                sound_play(SFX_CANTO);

                if (chosen_kind == KIND_TRUCO) {
                    signed char truco_delta = truco_choose(&truco_chain, chosen_code, cpu_best_remaining_power(),
                                                            &truco_last_singer, &offer_envido, player_hand, cpu_hand,
                                                            award_points_and_flash, falta_target(),
                                                            (unsigned char)(hand_mano == TRICK_PLAYER));
                    refresh_stake(); /* el truco aceptado cambio cuanto vale la mano */
                    /* una vez que se negocia un canto de truco (aceptado,
                       o la mano termina), el envido ya no se puede cantar
                       mas (ver truco_ui.h). */
                    offer_envido = 0;
                    if (truco_delta != 0) {
                        *hand_ended = 1;
                        *early_points = truco_delta;
                        return TRICK_TIE; /* no se usa: la mano termino por un canto, no por la baza */
                    }
                } else {
                    /* envido/real/falta: siempre queda resuelto del todo
                       aca mismo (no afecta como se sigue jugando esta
                       baza), asi que ya no se vuelve a ofrecer en esta
                       mano (offer_envido se apaga). */
                    signed char envido_delta = canto_choose(chosen_code, player_hand, cpu_hand, falta_target(),
                                                             (unsigned char)(hand_mano == TRICK_PLAYER));
                    if (envido_delta != 0) award_points_and_flash(envido_delta);
                    offer_envido = 0;
                }

                /* recalcular las opciones (puede haber cambiado lo que se
                   puede cantar de truco, o el envido ya no se ofrece mas)
                   y volver al modo carta. */
                canto_count = build_canto_menu(offer_envido, canto_kind, canto_code);
                in_menu = 0;
                menu_sel = 0;
                draw_canto_menu(canto_kind, canto_code, canto_count, 1);
                wait_vblank();
                set_card_cursor(player_cursor, 1);
                ppu_finish_vram_update(0x80);
            }
        }
    }

    draw_canto_menu(canto_kind, canto_code, canto_count, 0);

    /* Esta tanda (borrar cursor y carta de la mano + marco, dibujar las 2
       cartas jugadas) son unos 19 tiles: de sobra para no entrar siempre
       en una sola ventana de vblank (eso a veces corrompia la pantalla —
       el mismo problema que los menus, ver canto_ui.c). La hacemos con el
       render apagado, como al repartir.
       Las cartas jugadas van en la columna de ESTA baza (HAND_COL[trick]),
       fila CPU arriba y fila jugador abajo, y quedan ahi toda la mano: asi
       se puede comparar de un vistazo que carta gano cada baza (en vez de
       una "mesa" compartida que se borraba entre bazas). */
    PPU.mask = 0x00;
    set_card_cursor(player_cursor, 0);

    player_card = player_hand[player_cursor];
    player_played[player_cursor] = 1;
    clear_card(HAND_COL[player_cursor], PLAYER_ROW);
    clear_card_border(HAND_COL[player_cursor], PLAYER_ROW);
    draw_card_face(HAND_COL[trick], PLAYER_PLAYED_ROW, player_card);
    draw_card_border(HAND_COL[trick], PLAYER_PLAYED_ROW);

    /* Si abrio el jugador, recien ahora le toca hablar a la CPU (ver
       cpu_try_cantar()): si el jugador ya jugo su carta sin cantar nada,
       es el ultimo momento en que la CPU puede cantar en vez de responder
       con una carta. Si no canta (o canta pero la mano sigue), elige con
       la IA (Fase 7, game/ai.c) ya sabiendo la carta del jugador: gana con
       la mas floja que le alcance, o sacrifica la mas floja si no puede
       ganar la baza. Si abrio la CPU, su carta ya se elgio y se dibujo
       mas arriba (y ya tuvo su chance de cantar antes de eso). */
    if (leader == TRICK_PLAYER) {
        if (cpu_try_cantar(&offer_envido, hand_ended, early_points)) {
            /* el render seguia apagado desde la tanda de dibujar la carta
               del jugador un poco mas arriba: hay que prenderlo de nuevo
               antes de volver, si no el resto de la pantalla (marcador,
               flash de color) queda a oscuras. */
            ppu_reset_scroll();
            PPU.control = 0x80;
            PPU.mask = 0x1E;
            return TRICK_TIE; /* no se usa: la mano termino por un canto, no por la baza */
        }
        cpu_idx = ai_choose_card(cpu_hand, cpu_played, player_card);
        cpu_card = cpu_hand[cpu_idx];
        cpu_played[cpu_idx] = 1;

        /* cpu_try_cantar() puede haber corrido una negociacion (mensaje
           "CPU CANTO X", menu de respuesta) que deja el render PRENDIDO al
           volver (termina con PPU.mask=0x1E); hay que apagarlo de nuevo
           antes de esta tanda de escrituras a VRAM, si no quedan
           corruptas de forma intermitente (mismo problema de fondo que el
           bug de pantalla en blanco documentado en GRAFICOS.md). */
        PPU.mask = 0x00;
        clear_card_back(HAND_COL[cpu_idx], CPU_ROW);
        draw_card_face(HAND_COL[trick], CPU_PLAYED_ROW, cpu_card);
        draw_card_border(HAND_COL[trick], CPU_PLAYED_ROW);
    }
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;

    wait_frames(WAIT_AFTER_TRICK_FRAMES);

    return trick_result(player_card, cpu_card);
}

/* Juega una mano entera (cantos + bazas) y devuelve los puntos ganados en
   ella: positivo si son del jugador, negativo si son de la CPU, 0 en una
   mano parda. Ya deja aplicados esos puntos al marcador y muestra el
   flash de color correspondiente. */
static signed char play_hand(void)
{
    unsigned char trick;
    unsigned char hand_ended;
    unsigned char offer_envido;
    unsigned char leader = hand_mano; /* quien habla/tira primero en la 1ra baza (rota entre manos, ver main()) */
    signed char early_points;
    signed char hand_points = 0;
    unsigned char mano_is_player = (unsigned char)(hand_mano == TRICK_PLAYER);

    results[0] = TRICK_TIE;
    results[1] = TRICK_TIE;
    results[2] = TRICK_TIE;

    /* Si alguno tiene flor, se resuelve sola (no hay menu, no se puede
       achicar teniendo flor): esos puntos se aplican ya mismo y el envido
       ya no se ofrece en esta mano. Si nadie tiene flor (o se elgio jugar
       "sin flor" en title_screen()), el envido se ofrece integrado al
       cursor de cartas de la 1ra baza (ver play_trick()), nunca como un
       menu aparte que haya que "pasar". */
    if (flor_enabled) {
        signed char flor_points;
        if (canto_try_flor(player_hand, cpu_hand, &flor_points, falta_target(), mano_is_player)) {
            offer_envido = 0;
            if (flor_points != 0) award_points_and_flash(flor_points);
        } else {
            offer_envido = 1;
        }
    } else {
        offer_envido = 1;
    }

    hand_ended = 0;
    early_points = 0;
    for (trick = 0; trick < 3 && !hand_ended; ++trick) {
        unsigned char r = play_trick(trick, (unsigned char)(trick == 0 && offer_envido), leader, &hand_ended, &early_points);
        if (!hand_ended) {
            results[trick] = r;
            draw_trick_mark(trick, r);
            /* el que gana la baza tira primero la siguiente; si fue
               parda, sigue abriendo el mismo (ver REGLAS.md). */
            if (r != TRICK_TIE) leader = r;
            /* si ya se sabe quien gana la mano con 2 bazas, no hace falta
               jugar la 3ra (ver hand_decided_after_two() en match.c). */
            if (trick == 1 && hand_decided_after_two(results)) {
                break;
            }
        }
    }

    if (hand_ended) {
        hand_points = early_points;
    } else {
        unsigned char winner = hand_winner(results);
        unsigned char value = truco_hand_value(&truco_chain);
        if (winner == TRICK_PLAYER) hand_points = (signed char)value;
        else if (winner == TRICK_CPU) hand_points = (signed char)(-value);
    }

    apply_points(hand_points);
    refresh_scoreboard();

    wait_vblank();
    if (hand_points > 0) {
        ppu_set_backdrop(BACKDROP_WIN);
        sound_play(SFX_WIN);
    } else if (hand_points < 0) {
        ppu_set_backdrop(BACKDROP_LOSE);
        sound_play(SFX_LOSE);
    } else {
        ppu_set_backdrop(BACKDROP_TIE);
    }
    ppu_finish_vram_update(0x80);
    if (hand_points > 0) {
        music_play_win();
    } else if (hand_points < 0) {
        music_play_lose();
    } else {
        wait_frames(WAIT_AFTER_TRICK_FRAMES);
    }
    wait_vblank();
    ppu_set_backdrop(BACKDROP_NEUTRAL);
    ppu_finish_vram_update(0x80);

    return hand_points;
}

/* Las 4 combinaciones elegibles en la pantalla de inicio (ver
   title_screen()): con/sin flor (game/canto.c: con flor habilitada, una
   mano con 3 cartas del mismo palo se resuelve sola en vez de poder
   cantar envido, ver canto_try_flor()) y a 15 o 30 puntos (target_score,
   usado en main() y en falta_target() para "falta envido"/"contraflor al
   resto"). */
static const unsigned char START_FLOR[NUM_START_OPTIONS]  = { 1, 1, 0, 0 };
static const unsigned char START_SCORE[NUM_START_OPTIONS] = { 15, 30, 15, 30 };

static void draw_start_option(unsigned char row, unsigned char flor, unsigned char score)
{
    draw_text(TITLE_MENU_COL, row, flor ? "CON FLOR A" : "SIN FLOR A");
    draw_number((unsigned char)(TITLE_MENU_COL + 11), row, score);
}

/* Pantalla de inicio: titulo, creditos, y el menu de arriba para elegir
   antes de arrancar. El dibujado inicial es con el render apagado (se
   hace una sola vez, no hace falta sincronizar nada con el vblank); la
   navegacion del menu si va cuadro a cuadro, mismo patron que el resto de
   los menus del juego. */
static void title_screen(void)
{
    unsigned char pressed;
    unsigned char sel = 0;
    unsigned char i;

    PPU.mask = 0x00;
    ppu_clear_nametable0();

    draw_frame(TITLE_FRAME_COL, TITLE_FRAME_ROW, TITLE_FRAME_W, TITLE_FRAME_H);

    draw_text(TITLE_COL, TITLE_ROW, "TRUCO");
    ppu_set_tile((unsigned char)(TITLE_COL + 5), TITLE_ROW, digit_tile(8));
    ppu_set_tile((unsigned char)(TITLE_COL + 6), TITLE_ROW, digit_tile(6));

    draw_text(TITLE_CREDIT_COL, TITLE_CREDIT_ROW, "CODE.AR");

    /* Los 4 palos GRANDES (16x16), cada uno de su color: es el mismo dibujo
       que despues se ve en las cartas, asi que la pantalla de inicio ya
       "ensena" que forma y que color tiene cada palo. */
    for (i = 0; i < 4; ++i) {
        unsigned char col = (unsigned char)(TITLE_SUITS_COL + i * TITLE_SUITS_STEP);
        unsigned char base = (unsigned char)(TILE_SUIT_BIG_BASE + i * 4);
        unsigned char pal = (i == 0) ? PALETTE_ESPADA
                          : (i == 1) ? PALETTE_BASTO
                          : (i == 2) ? PALETTE_ORO
                                     : PALETTE_DEFAULT; /* copa: ya es roja en la paleta 0 */
        ppu_set_tile(col, TITLE_SUITS_ROW, base);
        ppu_set_tile((unsigned char)(col + 1), TITLE_SUITS_ROW, (unsigned char)(base + 1));
        ppu_set_tile(col, (unsigned char)(TITLE_SUITS_ROW + 1), (unsigned char)(base + 2));
        ppu_set_tile((unsigned char)(col + 1), (unsigned char)(TITLE_SUITS_ROW + 1), (unsigned char)(base + 3));
        ppu_set_palette_block(col, TITLE_SUITS_ROW, pal);
        ppu_set_palette_block(col, (unsigned char)(TITLE_SUITS_ROW + 1), pal);
    }

    for (i = 0; i < NUM_START_OPTIONS; ++i) {
        draw_start_option((unsigned char)(TITLE_MENU_ROW + i * TITLE_MENU_STEP), START_FLOR[i], START_SCORE[i]);
    }
    ppu_set_tile(TITLE_MENU_CURSOR_COL, TITLE_MENU_ROW, TILE_ARROW_RIGHT);

    ppu_set_backdrop(BACKDROP_NEUTRAL);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;

    music_init();
    for (;;) {
        wait_vblank();
        music_update();
        input_update();
        pressed = input_pressed();

        if (pressed & JOY_UP_MASK) {
            ppu_set_tile(TITLE_MENU_CURSOR_COL, (unsigned char)(TITLE_MENU_ROW + sel * TITLE_MENU_STEP), TILE_BLANK);
            sel = (sel == 0) ? (unsigned char)(NUM_START_OPTIONS - 1) : (unsigned char)(sel - 1);
            ppu_set_tile(TITLE_MENU_CURSOR_COL, (unsigned char)(TITLE_MENU_ROW + sel * TITLE_MENU_STEP), TILE_ARROW_RIGHT);
            ppu_finish_vram_update(0x80);
            sound_play(SFX_MOVE);
        } else if (pressed & JOY_DOWN_MASK) {
            ppu_set_tile(TITLE_MENU_CURSOR_COL, (unsigned char)(TITLE_MENU_ROW + sel * TITLE_MENU_STEP), TILE_BLANK);
            sel = (unsigned char)(sel + 1);
            if (sel >= NUM_START_OPTIONS) sel = 0;
            ppu_set_tile(TITLE_MENU_CURSOR_COL, (unsigned char)(TITLE_MENU_ROW + sel * TITLE_MENU_STEP), TILE_ARROW_RIGHT);
            ppu_finish_vram_update(0x80);
            sound_play(SFX_MOVE);
        } else if (pressed & (JOY_START_MASK | JOY_BTN_A_MASK)) {
            break;
        }
    }
    music_stop();
    sound_play(SFX_CONFIRM);

    flor_enabled = START_FLOR[sel];
    target_score = START_SCORE[sel];
}

void main(void)
{
    unsigned char pressed;

    input_init();
    sound_init();

    PPU.control = 0x00;
    PPU.mask = 0x00; /* render apagado: momento seguro para escribir VRAM */
    ppu_load_palette(palette);

    for (;;) {
        title_screen();

        score_player = 0;
        score_cpu = 0;
        hand_mano = TRICK_PLAYER; /* el jugador es mano en la 1ra mano de cada partida */

        while (score_player < target_score && score_cpu < target_score) {
            deal_hand();
            play_hand();
            /* la mano (quien habla/tira primero) rota cada mano, sea cual
               sea el resultado de la anterior. */
            hand_mano = (hand_mano == TRICK_PLAYER) ? TRICK_CPU : TRICK_PLAYER;
        }

        /* pantalla final: marcador + GANASTE/PERDISTE (todo con el render
           apagado, incluido el color de fondo, para no tener que
           sincronizar nada con el vblank). Se queda esperando Start/A
           para volver a la pantalla de inicio (antes se quedaba
           trabada ahi para siempre). */
        {
            unsigned char won = (unsigned char)(score_player >= target_score);
            PPU.mask = 0x00;
            ppu_clear_nametable0();
            draw_frame(RESULT_FRAME_COL, RESULT_FRAME_ROW, RESULT_FRAME_W, RESULT_FRAME_H);
            if (won) {
                draw_text(RESULT_COL, RESULT_ROW, "GANASTE");
            } else {
                draw_text((unsigned char)(RESULT_COL - 1), RESULT_ROW, "PERDISTE");
            }
            /* marcador final adentro del marco, para cerrar la partida con el
               resultado a la vista y no solo con la palabra. */
            draw_text(RESULT_VOS_COL, RESULT_SCORE_ROW, "VOS");
            draw_number((unsigned char)(RESULT_VOS_COL + 4), RESULT_SCORE_ROW, score_player);
            draw_text(RESULT_CPU_COL, RESULT_SCORE_ROW, "CPU");
            draw_number((unsigned char)(RESULT_CPU_COL + 4), RESULT_SCORE_ROW, score_cpu);
            ppu_set_backdrop(won ? BACKDROP_WIN : BACKDROP_LOSE);
        }
        ppu_reset_scroll();
        PPU.control = 0x80;
        PPU.mask = 0x1E;

        for (;;) {
            wait_vblank();
            input_update();
            pressed = input_pressed();
            if (pressed & (JOY_START_MASK | JOY_BTN_A_MASK)) break;
        }
        sound_play(SFX_CONFIRM);
    }
}
