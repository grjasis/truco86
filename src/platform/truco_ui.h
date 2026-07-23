#ifndef TRUCO86_TRUCO_UI_H
#define TRUCO86_TRUCO_UI_H

#include "../game/canto.h"
#include "../game/deck.h"

/* Se la pasa a truco_choose()/truco_cpu_initiates() para que, si alguien
 * canta envido mientras responde a un canto de truco pendiente, puedan
 * avisarle a quien las llama que sume/reste esos puntos ya mismo (ver la
 * nota larga mas abajo sobre la prioridad del envido). */
typedef void (*TrucoAwardPointsFn)(signed char delta);

#define TRUCO_MAX_OPTIONS 4

/* Quien canto el ultimo escalon aceptado (quiero) de la cadena de truco.
 * Solo el que NO figura aca puede cantar el siguiente escalon (no se
 * puede "auto-escalar" el propio canto que el rival ya acepto): ver
 * truco_list_options()/truco_choose()/truco_cpu_initiates(). NONE es el
 * valor inicial de cada mano (nadie canto todavia, cualquiera puede
 * abrir). */
#define TRUCO_SINGER_PLAYER 0
#define TRUCO_SINGER_CPU    1
#define TRUCO_SINGER_NONE   2

/* Opciones de canto disponibles AHORA para que el jugador cante por
 * primera vez en esta baza (Truco/Retruco/Vale Cuatro, el que corresponda,
 * e Irse al mazo). No incluye "Paso": si el jugador no quiere cantar
 * nada, simplemente elige jugar una carta en vez de entrar a este menu
 * (ver main.c, que combina este menu con la seleccion de carta en un solo
 * cursor). 'can_escalate' es 0 si el jugador fue quien canto el ultimo
 * escalon aceptado (le toca esperar a que responda/escale la CPU): en ese
 * caso no se ofrece ningun escalon, solo "Irse al mazo" (que siempre esta
 * disponible). Devuelve la cantidad de opciones (siempre al menos 1) y las
 * deja en out[]. */
unsigned char truco_list_options(const TrucoChain *chain, unsigned char can_escalate, unsigned char out[TRUCO_MAX_OPTIONS]);

/* Si hay un escalon legal para cantar ahora (Truco/Retruco/Vale Cuatro
 * segun el estado de la cadena), lo deja en *out_code y devuelve 1; si no
 * hay ninguno mas (ya se canto Vale Cuatro), devuelve 0. No tiene en
 * cuenta 'TRUCO_SINGER_*' (eso lo evalua el que llama, ver
 * truco_list_options() y la iniciativa de la CPU en main.c). */
unsigned char truco_next_escalation(const TrucoChain *chain, unsigned char *out_code);

/* Texto para mostrar de una opcion devuelta por truco_list_options(). */
const char *truco_option_text(unsigned char code);

/* La CPU decide si toma la iniciativa y canta ella misma el proximo
 * escalon posible (Truco/Retruco/Vale Cuatro), en vez de esperar a que el
 * jugador cante algo (ver ai_truco_escalate() en game/ai.c). 'can_initiate'
 * es 0 si la CPU fue quien canto el ultimo escalon aceptado (le toca
 * esperar al jugador, mismo criterio que 'can_escalate' de
 * truco_list_options()). Si decide cantar, devuelve 1 con el codigo
 * elegido en *out_code (para pasarle a truco_cpu_initiates()). */
unsigned char truco_cpu_wants_to_initiate(const TrucoChain *chain, unsigned char cpu_best_power,
                                           unsigned char entropy, unsigned char can_initiate,
                                           unsigned char *out_code);

/* El jugador canta la opcion 'code' (una de las que devolvio
 * truco_list_options). Corre el resto de la negociacion: la CPU responde o
 * escala con la IA de game/ai.c, alternando turnos con el jugador (con un
 * menu propio, Quiero/No/escalar) si hace falta.
 *
 * Prioridad del envido sobre el truco (regla real): mientras el truco
 * cantado todavia no fue aceptado (nadie dijo "quiero" a ese escalon),
 * quien tiene que responder (jugador o CPU) puede cantar envido en vez de
 * responder al truco. *envido_available indica si el envido todavia se
 * puede cantar esta mano (se apaga solo, tanto si se usa aca como si el
 * truco termina aceptado: una vez que el truco esta "querido", el envido
 * ya no se puede cantar mas). Si alguien lo usa, se resuelve ahi mismo
 * (con player_hand/cpu_hand) y se le avisan los puntos a 'award_points'
 * antes de seguir preguntando la respuesta real al truco pendiente.
 * 'falta_target' es cuanto le falta al que va ganando la partida para
 * llegar al maximo (ver main.c), para poder resolver un "falta envido"
 * cantado en el medio.
 *
 * Devuelve:
 *   0    -> la cadena queda escalada (alguien dijo "quiero") y la mano
 *           sigue jugandose con el nuevo valor; *out_last_singer queda en
 *           quien canto ese ultimo escalon aceptado (para la proxima
 *           llamada a truco_list_options()/truco_cpu_initiates()).
 *   !=0  -> la mano termina ya (no quiero, o alguien se fue al mazo):
 *           positivo si los puntos son del jugador, negativo si son de la
 *           CPU. (Esto no incluye los puntos de un envido cantado en el
 *           medio: esos ya se avisaron por separado via award_points).
 *
 * 'mano_is_player' dice quien es mano esta mano (ver hand_mano en main.c):
 * lo necesita un empate de puntos si se interrumpe con envido en el medio. */
signed char truco_choose(TrucoChain *chain, unsigned char code, unsigned char cpu_best_power,
                          unsigned char *out_last_singer, unsigned char *envido_available,
                          const Card player_hand[3], const Card cpu_hand[3],
                          TrucoAwardPointsFn award_points, unsigned char falta_target,
                          unsigned char mano_is_player);

/* Igual que truco_choose(), pero es la CPU la que canta primero (iniciativa
 * propia, ver ai_truco_escalate() en game/ai.c llamado desde main.c): el
 * jugador responde con un menu Quiero/No/escalar (o envido, ver arriba).
 * Mismos valores de retorno que truco_choose(). */
signed char truco_cpu_initiates(TrucoChain *chain, unsigned char code, unsigned char cpu_best_power,
                                 unsigned char *out_last_singer, unsigned char *envido_available,
                                 const Card player_hand[3], const Card cpu_hand[3],
                                 TrucoAwardPointsFn award_points, unsigned char falta_target,
                                 unsigned char mano_is_player);

#endif
