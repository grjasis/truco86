#ifndef TRUCO86_CANTO_UI_H
#define TRUCO86_CANTO_UI_H

#include "../game/deck.h"

#define CANTO_MAX_OPTIONS 3

/* Si alguno de los dos tiene flor, la resuelve automaticamente (con flor
 * no se puede "achicar", asi que no hay menu interactivo) y devuelve 1 con
 * la diferencia de puntos en *out_points (positivo si los gana el jugador,
 * negativo si los gana la CPU). Si ninguno tiene flor, devuelve 0 y no
 * toca *out_points: el llamador debe ofrecer el envido con
 * canto_list_options()/canto_choose(), igual que el truco (ver
 * truco_ui.h), integrado al cursor de cartas de la primera baza.
 * 'falta_target' es cuanto le falta al que va ganando la partida para
 * llegar al maximo (ver main.c): lo usan "contraflor al resto" (aca) y
 * "falta envido" (en canto_choose()/canto_cpu_initiates() mas abajo).
 * 'mano_is_player' dice quien es mano esta mano (ver hand_mano en main.c):
 * un empate de puntos lo gana quien sea mano. */
unsigned char canto_try_flor(const Card player_hand[3], const Card cpu_hand[3], signed char *out_points,
                              unsigned char falta_target, unsigned char mano_is_player);

/* Opciones de envido disponibles AHORA para que el jugador cante por
 * primera vez (Envido/Real envido/Falta envido, los que correspondan
 * segun las reglas). No incluye "Paso": si el jugador no quiere cantar
 * envido, simplemente elige jugar una carta en vez de entrar a este menu
 * (ver main.c, que combina este menu con la seleccion de carta y el de
 * truco en un solo cursor). Devuelve la cantidad de opciones en out[]. */
unsigned char canto_list_options(unsigned char out[CANTO_MAX_OPTIONS]);

/* Texto para mostrar de una opcion devuelta por canto_list_options(). */
const char *canto_option_text(unsigned char code);

/* La CPU decide si toma la iniciativa y abre el envido ella misma (en vez
 * de esperar a que el jugador cante algo), y con que (envido o real
 * envido) segun su propio puntaje (ver ai_envido_accept()/
 * ai_envido_escalate() en game/ai.c). Solo tiene sentido llamarla cuando
 * nadie tiene flor y todavia no se canto nada de envido en esta mano. Si
 * decide abrir, devuelve 1 con el codigo elegido en *out_code (para
 * pasarle a canto_cpu_initiates()). */
unsigned char canto_cpu_wants_to_initiate(unsigned char cpu_envido, unsigned char entropy, unsigned char *out_code);

/* El jugador canta la opcion 'code' (una de las que devolvio
 * canto_list_options). Corre el resto de la negociacion (la CPU responde o
 * escala con la IA de game/ai.c, alternando turnos con el jugador con un
 * menu propio Quiero/No/escalar) y devuelve la diferencia de puntos final:
 * positivo si los gana el jugador, negativo si los gana la CPU. A
 * diferencia del truco, el envido siempre queda resuelto del todo en esta
 * llamada (no hay "sigue jugandose con el valor nuevo": el envido no
 * afecta como se juegan las bazas). */
signed char canto_choose(unsigned char code, const Card player_hand[3], const Card cpu_hand[3],
                          unsigned char falta_target, unsigned char mano_is_player);

/* Igual que canto_choose(), pero es la CPU la que canta primero
 * (iniciativa propia, ver ai_envido_escalate()/ai_envido_accept() en
 * game/ai.c llamado desde main.c): el jugador responde con un menu
 * Quiero/No/escalar. Solo tiene sentido llamarla cuando nadie tiene flor
 * y todavia no se canto nada de envido en esta mano. Mismo valor de
 * retorno que canto_choose(). */
signed char canto_cpu_initiates(unsigned char code, const Card player_hand[3], const Card cpu_hand[3],
                                 unsigned char falta_target, unsigned char mano_is_player);

#endif
