#ifndef TRUCO86_INPUT_H
#define TRUCO86_INPUT_H

/* Instala el driver de joystick 1. Llamar una sola vez al arrancar. */
void input_init(void);

/* Hay que llamarla una vez por frame (despues de wait_vblank). Actualiza el
 * estado leido y calcula que botones se acaban de apretar (flanco). */
void input_update(void);

/* Estado actual del control (bits JOY_* de joystick.h), nivel. */
unsigned char input_held(void);

/* Botones que pasaron de sueltos a apretados en este ultimo input_update(). */
unsigned char input_pressed(void);

#endif
