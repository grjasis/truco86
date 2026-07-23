#ifndef TRUCO86_CANTO_H
#define TRUCO86_CANTO_H

/* Motor de puntos de los cantos de Envido y Flor. Puro (sin hardware),
 * testeable en el host. No decide QUIEN puede cantar que ni maneja turnos
 * (eso es responsabilidad de la capa de juego); solo calcula, dado el
 * estado de la cadena de cantos, cuanto vale aceptarla (quiero) o
 * rechazarla (no quiero). Ver docs/REGLAS.md para la tabla de valores.
 */

/* --- Envido: envido / envido-envido / real envido / falta envido --- */

typedef struct {
    unsigned char envidos;      /* cuantas veces se canto "envido": 0, 1 o 2 */
    unsigned char real_envido;  /* 1 si ya se canto real envido */
    unsigned char falta_envido; /* 1 si el ultimo canto fue falta envido (cierra la cadena) */
} EnvidoChain;

/* Puntos si se acepta (quiero) el ultimo canto de la cadena. falta_target
 * es cuanto le falta al que va ganando la partida para llegar al maximo
 * (solo se usa si chain->falta_envido). */
unsigned char envido_quiero_value(const EnvidoChain *chain, unsigned char falta_target);

/* Puntos para quien canto el ultimo escalon de la cadena, si el rival dice
 * "no quiero". 'before' es la cadena ANTES de ese ultimo canto (con la
 * cadena inicial {0,0,0} el valor es 1, el minimo). */
unsigned char envido_no_quiero_value(const EnvidoChain *before);

unsigned char envido_can_sing_envido(const EnvidoChain *chain);
unsigned char envido_can_sing_real(const EnvidoChain *chain);
unsigned char envido_can_sing_falta(const EnvidoChain *chain);

/* --- Flor: flor / contraflor / contraflor al resto --- */

typedef struct {
    unsigned char step; /* 0=nada, 1=flor, 2=contraflor, 3=contraflor al resto */
} FlorChain;

#define FLOR_STEP_NONE             0
#define FLOR_STEP_FLOR             1
#define FLOR_STEP_CONTRAFLOR       2
#define FLOR_STEP_CONTRAFLOR_RESTO 3

unsigned char flor_quiero_value(const FlorChain *chain, unsigned char falta_target);
unsigned char flor_no_quiero_value(const FlorChain *before);

unsigned char flor_can_sing_contraflor(const FlorChain *chain);
unsigned char flor_can_sing_contraflor_resto(const FlorChain *chain);

/* --- Truco: truco / retruco / vale cuatro --- */

typedef struct {
    unsigned char step; /* 0=nada (la mano vale 1), 1=truco, 2=retruco, 3=vale cuatro */
} TrucoChain;

#define TRUCO_STEP_NONE        0
#define TRUCO_STEP_TRUCO       1
#define TRUCO_STEP_RETRUCO     2
#define TRUCO_STEP_VALE_CUATRO 3

/* Cuanto vale la mano ahora mismo si nadie canta nada mas (1 si nunca se
 * canto truco). */
unsigned char truco_hand_value(const TrucoChain *chain);

/* Cuanto vale si se acepta (quiero) escalar la cadena al escalon dado
 * (TRUCO_STEP_TRUCO, _RETRUCO o _VALE_CUATRO). */
unsigned char truco_quiero_value(unsigned char new_step);

/* Cuanto se lleva quien canto el ultimo escalon, si el rival dice "no
 * quiero" (o se va al mazo respondiendo a ese canto). 'before' es la
 * cadena ANTES de ese escalon. */
unsigned char truco_no_quiero_value(const TrucoChain *before);

unsigned char truco_can_sing_truco(const TrucoChain *chain);
unsigned char truco_can_sing_retruco(const TrucoChain *chain);
unsigned char truco_can_sing_vale_cuatro(const TrucoChain *chain);

#endif
