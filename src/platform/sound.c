#include <nes.h>
#include "sound.h"

/* Un solo preset por efecto: los 4 bytes que se escriben en orden a
 * APU.pulse[0] ($4000-$4003). Escribir el ultimo ($4003) es lo que
 * relanza la nota (reinicia fase, recarga el contador de duracion y el
 * envelope), asi que el orden de los campos importa.
 *
 * control: duty(2) | halt(1)=0 | volumen_constante(1)=1 | volumen(4)
 * ramp:    sweep, siempre 0 (sin variacion de tono)
 * period_low / len_period_high: periodo del tono (altura) + indice de
 *   duracion (cuanto dura la nota, en unidades del contador de longitud
 *   de la APU).
 * Los periodos estan calculados para el clock de CPU de PAL (~1.66MHz),
 * que es el target de Truco86 (ver docs/ARQUITECTURA.md). */
typedef struct {
    unsigned char control;
    unsigned char ramp;
    unsigned char period_low;
    unsigned char len_period_high;
} PulsePreset;

static const PulsePreset PRESETS[6] = {
    /* SFX_MOVE:    blip corto y agudo, ~800Hz */
    { 0x96, 0x00, 0x81, 0x48 },
    /* SFX_CONFIRM: blip corto, ~1000Hz */
    { 0x98, 0x00, 0x66, 0x48 },
    /* SFX_CARD:    blip corto, ~900Hz */
    { 0x97, 0x00, 0x72, 0x48 },
    /* SFX_CANTO:   blip medio, duty distinto, ~600Hz */
    { 0x5A, 0x00, 0xAC, 0x68 },
    /* SFX_WIN:     nota larga y aguda, ~1500Hz */
    { 0xDC, 0x00, 0x43, 0xA0 },
    /* SFX_LOSE:    nota larga y grave, ~200Hz */
    { 0x1A, 0x00, 0x06, 0xA2 },
};

void sound_init(void)
{
    APU.fcontrol = 0x00; /* modo de 4 pasos, sin IRQ del frame counter */
    APU.status = 0x01;   /* habilita solo el canal de pulso 1 */
}

void sound_play(SfxId id)
{
    const PulsePreset *p = &PRESETS[id];
    APU.pulse[0].control = p->control;
    APU.pulse[0].ramp = p->ramp;
    APU.pulse[0].period_low = p->period_low;
    APU.pulse[0].len_period_high = p->len_period_high;
}
