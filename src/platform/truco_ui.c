#include <nes.h>
#include "truco_ui.h"
#include "canto_ui.h"
#include "../game/ai.h"
#include "../game/envido.h"
#include "ppu_draw.h"
#include "cards_render.h"
#include "text.h"
#include "vsync.h"
#include "input.h"
#include "sound.h"

/* Ver la nota larga en truco_ui.h sobre la prioridad del envido: estas
 * opciones se mezclan con las de truco en el menu de RESPUESTA a un canto
 * pendiente (nunca en el de cantar por primera vez, truco_list_options()),
 * asi que necesitan distinguirse por 'kind' igual que en main.c. */
#define KIND_TRUCO  0
#define KIND_ENVIDO 1

/* Menu propio para cuando el jugador tiene que RESPONDER un canto de la
 * CPU (Quiero/No/escalar): esto es menos frecuente que cantar por primera
 * vez (que ahora esta integrado con la seleccion de carta en main.c), asi
 * que sigue siendo una pantalla de menu aparte, mismo patron que
 * canto_ui.c. */
#define MENU_COL 11
#define MENU_CURSOR_COL 9
#define MENU_ROW 16

#define MSG_ROW 15
#define MSG_COL 9
#define MSG_WAIT_FRAMES 60 /* ~1.2s a 50 cuadros/seg, para poder leer que canto la CPU */

/* Las opciones se representan como unsigned char (no como un enum de C,
 * que cc65 puede reservar como int de 2 bytes) para poder pasarlas tal
 * cual a traves de truco_list_options()/truco_choose() sin reinterpretar
 * el array (eso causaba un bug real: escribir "MenuOption" de 2 bytes en
 * un buffer de unsigned char corria las opciones siguientes). */
#define OPT_TRUCO   0
#define OPT_RETRUCO 1
#define OPT_CUATRO  2
#define OPT_QUIERO  3
#define OPT_NO      4
#define OPT_IRSE    5

static const char *const OPTION_TEXT[] = {
    "TRUCO", "RETRUCO", "CUATRO", "QUIERO", "NO", "IRSE"
};

unsigned char truco_next_escalation(const TrucoChain *chain, unsigned char *out_code)
{
    if (truco_can_sing_truco(chain))       { *out_code = OPT_TRUCO;   return 1; }
    if (truco_can_sing_retruco(chain))     { *out_code = OPT_RETRUCO; return 1; }
    if (truco_can_sing_vale_cuatro(chain)) { *out_code = OPT_CUATRO;  return 1; }
    return 0;
}

static unsigned char build_options(const TrucoChain *chain, unsigned char can_escalate, unsigned char can_answer, unsigned char out[4])
{
    unsigned char n = 0;
    unsigned char esc;

    if (can_escalate && truco_next_escalation(chain, &esc)) out[n++] = esc;
    if (can_answer) {
        out[n++] = OPT_QUIERO;
        out[n++] = OPT_NO;
    }
    out[n++] = OPT_IRSE;
    return n;
}

static void apply_escalation(TrucoChain *chain, unsigned char opt)
{
    if (opt == OPT_TRUCO) chain->step = TRUCO_STEP_TRUCO;
    else if (opt == OPT_RETRUCO) chain->step = TRUCO_STEP_RETRUCO;
    else if (opt == OPT_CUATRO) chain->step = TRUCO_STEP_VALE_CUATRO;
}

/* Menu de RESPUESTA a un canto de truco pendiente. Ver la nota larga en
 * truco_ui.h: mientras el canto pendiente todavia no fue aceptado, quien
 * responde puede cantar envido en cambio (si *envido_available) — se
 * mezcla con las opciones de truco (kind[]/code[] paralelos, mismo patron
 * que combina main.c). Si el jugador elige una opcion de envido, se
 * resuelve ahi mismo (canto_choose(), que puede a su vez abrir su propio
 * menu de respuesta si la CPU escala) y se vuelve a preguntar la
 * respuesta real al truco (por eso el bucle externo).
 *
 * Ver la nota larga en canto_ui.c: dibujar/borrar todas las opciones es una
 * tanda demasiado grande para una sola ventana de vblank (podia corromper
 * la pantalla), asi que esa parte apaga el render un instante; solo mover
 * el cursor (2 tiles) queda sincronizado a vblank con el render prendido. */
static unsigned char choose_response(const TrucoChain *chain, unsigned char *envido_available,
                                      const Card player_hand[3], const Card cpu_hand[3],
                                      TrucoAwardPointsFn award_points, unsigned char falta_target,
                                      unsigned char mano_is_player)
{
    for (;;) {
        unsigned char kind[TRUCO_MAX_OPTIONS + CANTO_MAX_OPTIONS];
        unsigned char code[TRUCO_MAX_OPTIONS + CANTO_MAX_OPTIONS];
        unsigned char n;
        unsigned char sel = 0;
        unsigned char i;
        unsigned char pressed;
        unsigned char chosen_kind;
        unsigned char chosen_code;

        n = build_options(chain, 1, 1, code);
        for (i = 0; i < n; ++i) kind[i] = KIND_TRUCO;

        if (*envido_available) {
            unsigned char envido_opts[CANTO_MAX_OPTIONS];
            unsigned char envido_n = canto_list_options(envido_opts);
            unsigned char j;
            for (j = 0; j < envido_n; ++j) {
                kind[n] = KIND_ENVIDO;
                code[n] = envido_opts[j];
                ++n;
            }
        }

        PPU.mask = 0x00;
        for (i = 0; i < n; ++i) {
            const char *text = (kind[i] == KIND_TRUCO) ? OPTION_TEXT[code[i]] : canto_option_text(code[i]);
            draw_text(MENU_COL, (unsigned char)(MENU_ROW + i), text);
        }
        ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_ARROW_RIGHT);
        ppu_reset_scroll();
        PPU.control = 0x80;
        PPU.mask = 0x1E;

        for (;;) {
            wait_vblank();
            input_update();
            pressed = input_pressed();

            if (pressed & JOY_UP_MASK) {
                unsigned char old_sel = sel;
                sel = (sel == 0) ? (unsigned char)(n - 1) : (unsigned char)(sel - 1);
                ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + old_sel), TILE_BLANK);
                ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_ARROW_RIGHT);
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if (pressed & JOY_DOWN_MASK) {
                unsigned char old_sel = sel;
                sel = (unsigned char)(sel + 1);
                if (sel >= n) sel = 0;
                ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + old_sel), TILE_BLANK);
                ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_ARROW_RIGHT);
                ppu_finish_vram_update(0x80);
                sound_play(SFX_MOVE);
            } else if (pressed & JOY_BTN_A_MASK) {
                sound_play(SFX_CONFIRM);
                break;
            }
        }

        chosen_kind = kind[sel];
        chosen_code = code[sel];

        PPU.mask = 0x00;
        for (i = 0; i < n; ++i) {
            clear_text(MENU_COL, (unsigned char)(MENU_ROW + i), 7);
        }
        ppu_set_tile(MENU_CURSOR_COL, (unsigned char)(MENU_ROW + sel), TILE_BLANK);
        ppu_reset_scroll();
        PPU.control = 0x80;
        PPU.mask = 0x1E;

        if (chosen_kind == KIND_ENVIDO) {
            signed char delta = canto_choose(chosen_code, player_hand, cpu_hand, falta_target, mano_is_player);
            if (delta != 0) award_points(delta);
            *envido_available = 0;
            continue; /* seguir preguntando: el canto de truco sigue pendiente */
        }

        return chosen_code;
    }
}

static void clear_cpu_message(void)
{
    PPU.mask = 0x00;
    clear_text(MSG_COL, MSG_ROW, 18);
    ppu_reset_scroll();
    PPU.control = 0x80;
    PPU.mask = 0x1E;
}

/* Deja "CPU CANTO <opcion>" en pantalla (no la borra sola): se queda
 * visible mientras el jugador responde en choose_option() (fila MENU_ROW,
 * distinta de MSG_ROW, asi que no se pisan), para que sepa que esta
 * respondiendo en vez de solo ver un menu de Quiero/No/Retruco sin
 * contexto. Se borra explicitamente en negotiate() cuando termina esta
 * negociacion, o antes de mostrar el proximo mensaje (ver arriba). */
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

/* Ver la nota larga en truco_ui.h: si el canto de truco pendiente todavia
 * no fue aceptado, la CPU tambien puede interrumpir con envido (con su
 * propia IA, mismo criterio que canto_cpu_wants_to_initiate() en
 * canto_ui.c) en vez de responder al truco. Si lo hace, el jugador
 * responde ese envido (canto_cpu_initiates()) y se avisan los puntos por
 * award_points(); devuelve 1 para que cpu_decide() vuelva a decidir su
 * respuesta real al truco. */
static unsigned char cpu_try_envido_interrupt(unsigned char *envido_available,
                                               const Card player_hand[3], const Card cpu_hand[3],
                                               TrucoAwardPointsFn award_points, unsigned char falta_target,
                                               unsigned char mano_is_player)
{
    unsigned char code;

    if (!*envido_available) return 0;
    if (!canto_cpu_wants_to_initiate(envido_score(cpu_hand), nes_frame_count, &code)) return 0;

    {
        signed char delta = canto_cpu_initiates(code, player_hand, cpu_hand, falta_target, mano_is_player);
        if (delta != 0) award_points(delta);
    }
    *envido_available = 0;
    return 1;
}

/* IA de la CPU (Fase 7, ver game/ai.c) para RESPONDER un canto pendiente:
 * acepta, rechaza o escala segun el poder de su mejor carta sin jugar.
 * nes_frame_count (platform/vsync.h) le da variedad a la decision cerca
 * del umbral. */
static unsigned char cpu_decide(const TrucoChain *chain, unsigned char cpu_best_power,
                                 unsigned char *envido_available,
                                 const Card player_hand[3], const Card cpu_hand[3],
                                 TrucoAwardPointsFn award_points, unsigned char falta_target,
                                 unsigned char mano_is_player)
{
    unsigned char esc;
    unsigned char choice;

    cpu_try_envido_interrupt(envido_available, player_hand, cpu_hand, award_points, falta_target, mano_is_player);

    if (ai_truco_escalate(cpu_best_power, nes_frame_count) && truco_next_escalation(chain, &esc)) {
        choice = esc;
    } else if (ai_truco_accept(cpu_best_power, nes_frame_count)) {
        choice = OPT_QUIERO;
    } else {
        choice = OPT_NO;
    }
    draw_cpu_message(choice);
    return choice;
}

unsigned char truco_list_options(const TrucoChain *chain, unsigned char can_escalate, unsigned char out[TRUCO_MAX_OPTIONS])
{
    return build_options(chain, can_escalate, 0, out);
}

const char *truco_option_text(unsigned char code)
{
    return OPTION_TEXT[code];
}

unsigned char truco_cpu_wants_to_initiate(const TrucoChain *chain, unsigned char cpu_best_power,
                                           unsigned char entropy, unsigned char can_initiate,
                                           unsigned char *out_code)
{
    if (!can_initiate) return 0;
    if (!truco_next_escalation(chain, out_code)) return 0;
    return ai_truco_escalate(cpu_best_power, entropy);
}

/* Negociacion compartida por truco_choose() (inicia el jugador) y
 * truco_cpu_initiates() (inicia la CPU): 'initiator_is_player' distingue
 * quien canta 'code' primero; a partir de ahi se alterna turno para
 * responder/escalar hasta que alguien dice quiero (devuelve 0, con
 * *out_last_singer = quien canto ese ultimo escalon) o no quiero/se va al
 * mazo (devuelve la diferencia de puntos final). Ver la nota larga en
 * truco_ui.h sobre 'envido_available'/'award_points' (prioridad del
 * envido mientras el canto de truco sigue sin responder). */
static signed char negotiate(TrucoChain *chain, unsigned char code, unsigned char cpu_best_power,
                              unsigned char initiator_is_player, unsigned char *out_last_singer,
                              unsigned char *envido_available, const Card player_hand[3], const Card cpu_hand[3],
                              TrucoAwardPointsFn award_points, unsigned char falta_target,
                              unsigned char mano_is_player)
{
    unsigned char choice = code;
    TrucoChain before;
    unsigned char player_turn;

    before = *chain;

    if (!initiator_is_player) {
        draw_cpu_message(choice);
    }

    apply_escalation(chain, choice);
    player_turn = (unsigned char)!initiator_is_player; /* le toca responder al otro */

    for (;;) {
        if (player_turn) {
            choice = choose_response(chain, envido_available, player_hand, cpu_hand, award_points, falta_target, mano_is_player);
        } else {
            choice = cpu_decide(chain, cpu_best_power, envido_available, player_hand, cpu_hand, award_points, falta_target, mano_is_player);
        }

        if (choice == OPT_QUIERO) {
            *out_last_singer = player_turn ? TRUCO_SINGER_CPU : TRUCO_SINGER_PLAYER;
            clear_cpu_message();
            return 0; /* la cadena queda escalada, se sigue jugando */
        }
        if (choice == OPT_NO || choice == OPT_IRSE) {
            unsigned char v = truco_no_quiero_value(&before);
            clear_cpu_message();
            return (signed char)(player_turn ? -v : v);
        }

        before = *chain;
        apply_escalation(chain, choice);
        player_turn = (unsigned char)!player_turn;
    }
}

signed char truco_choose(TrucoChain *chain, unsigned char code, unsigned char cpu_best_power,
                          unsigned char *out_last_singer, unsigned char *envido_available,
                          const Card player_hand[3], const Card cpu_hand[3],
                          TrucoAwardPointsFn award_points, unsigned char falta_target,
                          unsigned char mano_is_player)
{
    if (code == OPT_IRSE) {
        return (signed char)(-(signed char)truco_hand_value(chain));
    }
    return negotiate(chain, code, cpu_best_power, 1, out_last_singer, envido_available, player_hand, cpu_hand, award_points, falta_target, mano_is_player);
}

signed char truco_cpu_initiates(TrucoChain *chain, unsigned char code, unsigned char cpu_best_power,
                                 unsigned char *out_last_singer, unsigned char *envido_available,
                                 const Card player_hand[3], const Card cpu_hand[3],
                                 TrucoAwardPointsFn award_points, unsigned char falta_target,
                                 unsigned char mano_is_player)
{
    return negotiate(chain, code, cpu_best_power, 0, out_last_singer, envido_available, player_hand, cpu_hand, award_points, falta_target, mano_is_player);
}
