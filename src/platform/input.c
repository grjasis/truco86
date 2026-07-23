#include <joystick.h>
#include "input.h"

static unsigned char cur_state;
static unsigned char prev_state;
static unsigned char pressed_state;

void input_init(void)
{
    joy_install(joy_static_stddrv);
    cur_state = 0;
    prev_state = 0;
    pressed_state = 0;
}

void input_update(void)
{
    prev_state = cur_state;
    cur_state = joy_read(JOY_1);
    pressed_state = (unsigned char)(cur_state & (unsigned char)~prev_state);
}

unsigned char input_held(void)
{
    return cur_state;
}

unsigned char input_pressed(void)
{
    return pressed_state;
}
