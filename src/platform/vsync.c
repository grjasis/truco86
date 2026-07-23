#include "vsync.h"

void wait_vblank(void)
{
    unsigned char start = nes_frame_count;
    while (nes_frame_count == start) {
        /* espera al proximo NMI */
    }
}
