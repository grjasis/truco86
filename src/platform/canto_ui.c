#include <nes.h>
#include "canto_ui.h"
#include "../game/canto.h"
#include "../game/envido.h"
#include "../game/flor.h"
#include "../game/ai.h"
#include "ppu_draw.h"
#include "cards_render.h"
#include "text.h"
#include "vsync.h"
#include "input.h"
#include "sound.h"

/* Menu propio para cuando el jugador tiene que RESPONDER un canto de
 * envido pendiente (Quiero/No/escalar): igual que en truco_ui.c, esto es
 * menos frecuente que cantar por primera vez (que esta integrado con la
 * seleccion de carta en main.c), asi que sigue siendo una pantalla de menu
 * aparte. */
#define MENU_COL 11
#define MENU_CURSOR_COL 9
#define MENU_ROW 16

#define MSG_ROW 15
#define MSG_COL 9
#define MSG_WAIT_FRAMES 60 /* ~1.2s a 50 cuadros/seg, para poder leer que canto la CPU */
#define RESULT_WAIT_FRAMES 100 /* ~2s a 50 cuadros/seg, para poder leer el puntaje de cada uno */

/* Ver la nota larga en truco_ui.c: las opciones se representan como
 * unsigned char (no como un enum de C) para poder pasarlas tal cual a
 * traves de canto_list_options()/canto_choose() sin reinterpretar el
 * array. */
#define OPT_ENVIDO 0
#define OPT_REAL   1
#define OPT_FALTA  2
#define OPT_QUIERO 3
#define OPT_NO     4

static const char *const OPTION_TEXT[] = {
    "ENVIDO", "REAL", "FALTA", "QUIERO", "NO"
};

/* Muestra hasta 4 opciones y deja elegir con Up/Down + A. Solo se usa para
 * RESPONDER un canto pendiente (ver arriba); cantar por primera vez usa
 * canto_list_options()/canto_choose() desde main.c. */
static unsigned char choose_option(const unsigned char *opts, unsigned char count)
{
    unsigned char sel = 0;
    unsigned char i;
    unsigned char pressed;

    PPU.mask = 0x00;
    for (i = 0; i < count; ++i) {
        draw_text(MENU_COL, (unsigned char)(MENU_ROW + i), OPTION_TEXT[opts[i]]);
    }
    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_CURSOR);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;

    for (;;) {
        wait_vblank();
        input_update();
        pressed = input_pressed();

        if (pressed & JOY_UP_MASK) {
            unsigned char old_sel = sel;
            sel = (sel == 0) ? (unsigned char)(count - 1) : (unsigned char)(sel - 1);
            ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + old_sel), TILE_BLANK);
            ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_CURSOR);
            ppu_finish_vram_update(0x80);
            sound_play(SFX_MOVE);
        } else if (pressed & JOY_DOWN_MASK) {
            unsigned char old_sel = sel;
            sel = (unsigned char)(sel + 1);
            if (sel >= count) sel = 0;
            ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + old_sel), TILE_BLANK);
            ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_CURSOR);
            ppu_finish_vram_update(0x80);
            sound_play(SFX_MOVE);
        } else if (pressed & JOY_BTN_A_MASK) {
            sound_play(SFX_CONFIRM);
            break;
        }
    }

    PPU.mask = 0x00;
    for (i = 0; i < count; ++i) {
        clear_text(MENU_COL, (unsigned char)(MENU_ROW + i), 7);
    }
    ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_BLANK);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;

    return opts[sel];
}

/* Arma la lista de opciones legales para responder/escalar un canto ya
 * abierto (nunca la primera vez, ver canto_list_options()). */
static unsigned char build_options(const EnvidoChain *chain, unsigned char can_answer, unsigned char out[4])
{
    unsigned char n = 0;
    if (envido_can_sing_envido(chain)) out[n++] = OPT_ENVIDO;
    if (envido_can_sing_real(chain))   out[n++] = OPT_REAL;
    if (envido_can_sing_falta(chain))  out[n++] = OPT_FALTA;
    if (can_answer) {
        out[n++] = OPT_QUIERO;
        out[n++] = OPT_NO;
    }
    return n;
}

static void clear_cpu_message(void)
{
    PPU.mask = 0x00;
    clear_text(MSG_COL, MSG_ROW, 18);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;
}

/* Deja "CPU CANTO <opcion>" en pantalla (no la borra sola): ver la nota
 * larga en truco_ui.c (mismo patron) — se queda visible mientras el
 * jugador responde en choose_option(), y se borra en negotiate() cuando
 * termina esta negociacion. */
static void draw_cpu_message(unsigned char choice)
{
    unsigned char i;

    clear_cpu_message();
    PPU.mask = 0x00;
    draw_text(MSG_COL, MSG_ROW, "CPU CANTO");
    draw_text((unsigned char)(MSG_COL + 10), MSG_ROW, OPTION_TEXT[choice]);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;
    sound_play(SFX_CANTO);

    for (i = 0; i < MSG_WAIT_FRAMES; ++i) {
        wait_vblank();
    }
}

/* IA de la CPU (Fase 7, ver game/ai.c): responde o escala segun su propio
 * puntaje de envido. nes_frame_count (platform/vsync.h) le da variedad a
 * la decision cerca del umbral. 'falta_target' es cuanto le falta al que
 * va ganando la partida para llegar al maximo (ver main.c), para poder
 * evaluar si le conviene aceptar/escalar una "falta envido". */
static unsigned char cpu_decide(const EnvidoChain *chain, unsigned char cpu_envido, unsigned char falta_target)
{
    unsigned char choice;
    if (envido_can_sing_falta(chain) && ai_envido_escalate(cpu_envido, nes_frame_count)) {
        choice = envido_can_sing_real(chain) ? OPT_REAL : OPT_FALTA;
    } else if (ai_envido_accept(cpu_envido, envido_quiero_value(chain, falta_target), nes_frame_count)) {
        choice = OPT_QUIERO;
    } else {
        choice = OPT_NO;
    }
    draw_cpu_message(choice);
    return choice;
}

static void apply_canto(EnvidoChain *chain, unsigned char opt)
{
    if (opt == OPT_ENVIDO) chain->envidos++;
    else if (opt == OPT_REAL) chain->real_envido = 1;
    else if (opt == OPT_FALTA) chain->falta_envido = 1;
}

/* Muestra "CPU <ce> VOS <pe>" un rato: cuando se dice "quiero", las dos
 * manos se revelan y se comparan (a diferencia de un "no quiero", donde
 * los puntajes nunca se muestran porque nadie los revelo). */
static void show_envido_scores(unsigned char cpu_score, unsigned char player_score)
{
    unsigned char i;

    PPU.mask = 0x00;
    draw_text(MSG_COL, MSG_ROW, "CPU");
    draw_number((unsigned char)(MSG_COL + 4), MSG_ROW, cpu_score);
    draw_text((unsigned char)(MSG_COL + 7), MSG_ROW, "VOS");
    draw_number((unsigned char)(MSG_COL + 11), MSG_ROW, player_score);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;

    for (i = 0; i < RESULT_WAIT_FRAMES; ++i) {
        wait_vblank();
    }

    PPU.mask = 0x00;
    clear_text(MSG_COL, MSG_ROW, 15);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;
}

static signed char resolve_quiero(const EnvidoChain *chain, const Card player_hand[3], const Card cpu_hand[3],
                                   unsigned char falta_target, unsigned char mano_is_player)
{
    unsigned char value = envido_quiero_value(chain, falta_target);
    unsigned char pe = envido_score(player_hand);
    unsigned char ce = envido_score(cpu_hand);
    unsigned char player_wins = mano_is_player ? (pe >= ce) : (pe > ce);

    show_envido_scores(ce, pe);

    /* empate: gana quien sea mano esta mano (ver hand_mano en main.c). */
    return (signed char)(player_wins ? value : -value);
}

static signed char run_flor_phase(unsigned char player_flor, unsigned char cpu_flor,
                                   const Card player_hand[3], const Card cpu_hand[3],
                                   unsigned char falta_target, unsigned char mano_is_player)
{
    FlorChain solo = { FLOR_STEP_FLOR };
    FlorChain both = { FLOR_STEP_CONTRAFLOR };

    if (player_flor && !cpu_flor) {
        return (signed char)flor_quiero_value(&solo, falta_target);
    }
    if (cpu_flor && !player_flor) {
        return (signed char)(-flor_quiero_value(&solo, falta_target));
    }

    /* los dos tienen flor: no se puede "achicar" teniendo flor, asi que se
       resuelve directamente comparando valores (equivalente a que ambos
       digan quiero a contraflor). La UI interactiva de contraflor/
       contraflor al resto queda para una fase posterior. Empate: gana
       quien sea mano esta mano. */
    {
        unsigned char pf = flor_score(player_hand);
        unsigned char cf = flor_score(cpu_hand);
        unsigned char value = flor_quiero_value(&both, falta_target);
        unsigned char player_wins = mano_is_player ? (pf >= cf) : (pf > cf);
        return (signed char)(player_wins ? value : -value);
    }
}

unsigned char canto_try_flor(const Card player_hand[3], const Card cpu_hand[3], signed char *out_points,
                              unsigned char falta_target, unsigned char mano_is_player)
{
    unsigned char player_flor = flor_has(player_hand);
    unsigned char cpu_flor = flor_has(cpu_hand);

    if (!player_flor && !cpu_flor) return 0;

    *out_points = run_flor_phase(player_flor, cpu_flor, player_hand, cpu_hand, falta_target, mano_is_player);
    return 1;
}

unsigned char canto_list_options(unsigned char out[CANTO_MAX_OPTIONS])
{
    EnvidoChain chain = { 0, 0, 0 };
    unsigned char n = 0;
    if (envido_can_sing_envido(&chain)) out[n++] = OPT_ENVIDO;
    if (envido_can_sing_real(&chain))   out[n++] = OPT_REAL;
    if (envido_can_sing_falta(&chain))  out[n++] = OPT_FALTA;
    return n;
}

const char *canto_option_text(unsigned char code)
{
    return OPTION_TEXT[code];
}

unsigned char canto_cpu_wants_to_initiate(unsigned char cpu_envido, unsigned char entropy, unsigned char *out_code)
{
    if (ai_envido_escalate(cpu_envido, entropy)) {
        *out_code = OPT_REAL;
        return 1;
    }
    if (ai_envido_accept(cpu_envido, 0, entropy)) {
        *out_code = OPT_ENVIDO;
        return 1;
    }
    return 0;
}

/* Negociacion compartida por canto_choose() (inicia el jugador) y
 * canto_cpu_initiates() (inicia la CPU): 'initiator_is_player' distingue
 * quien canta 'code' primero; a partir de ahi se alterna turno para
 * responder/escalar hasta que alguien dice quiero (puntos finales,
 * resolviendo contra las manos) o no quiero (puntos del ultimo escalon). */
static signed char negotiate(unsigned char code, const Card player_hand[3], const Card cpu_hand[3],
                              unsigned char initiator_is_player, unsigned char falta_target,
                              unsigned char mano_is_player)
{
    EnvidoChain chain = { 0, 0, 0 };
    EnvidoChain before = { 0, 0, 0 };
    unsigned char choice = code;
    unsigned char player_turn;

    if (!initiator_is_player) {
        draw_cpu_message(choice);
    }

    apply_canto(&chain, choice);
    player_turn = (unsigned char)!initiator_is_player; /* le toca responder al otro */

    for (;;) {
        unsigned char opts[4];
        unsigned char n;

        if (player_turn) {
            n = build_options(&chain, 1, opts);
            choice = choose_option(opts, n);
        } else {
            choice = cpu_decide(&chain, envido_score(cpu_hand), falta_target);
        }

        if (choice == OPT_QUIERO) {
            clear_cpu_message();
            return resolve_quiero(&chain, player_hand, cpu_hand, falta_target, mano_is_player);
        }
        if (choice == OPT_NO) {
            unsigned char v = envido_no_quiero_value(&before);
            /* los puntos van para quien canto el ultimo escalon, es decir
               el que NO esta hablando ahora. */
            clear_cpu_message();
            return (signed char)(player_turn ? -v : v);
        }

        before = chain;
        apply_canto(&chain, choice);
        player_turn = (unsigned char)!player_turn;
    }
}

signed char canto_choose(unsigned char code, const Card player_hand[3], const Card cpu_hand[3],
                          unsigned char falta_target, unsigned char mano_is_player)
{
    return negotiate(code, player_hand, cpu_hand, 1, falta_target, mano_is_player);
}

signed char canto_cpu_initiates(unsigned char code, const Card player_hand[3], const Card cpu_hand[3],
                                 unsigned char falta_target, unsigned char mano_is_player)
{
    return negotiate(code, player_hand, cpu_hand, 0, falta_target, mano_is_player);
}
