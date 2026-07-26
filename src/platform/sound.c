#include <nes.h>
#include "sound.h"

/* Un solo preset por efecto: los 4 bytes que se escriben en orden al canal
 * que le toca. Escribir el ultimo registro es lo que relanza el efecto
 * (reinicia fase, recarga el contador de longitud y el envelope), asi que el
 * orden de los campos importa.
 *
 * Los efectos usan dos recursos del hardware que antes estaban sin tocar, y
 * que son lo que los saca de "beep plano":
 *
 * - ENVELOPE (bit de volumen constante en 0): en vez de sonar a volumen fijo
 *   y cortarse de golpe cuando se acaba el contador de longitud, la nota
 *   arranca en 15 y decae sola. Los 4 bits bajos dejan de ser el volumen y
 *   pasan a ser la velocidad de ese decaimiento (mas chico = mas rapido).
 * - BARRIDO / sweep (registro 'ramp'): la APU sube o baja el tono sola
 *   mientras suena. Un barrido hacia arriba suena afirmativo (confirmar,
 *   ganar) y uno hacia abajo suena a caida (cantar, perder).
 *
 * Ademas el sonido de jugar una carta y el de repartir pasaron al canal de
 * RUIDO: una carta que cae sobre la mesa es un golpe, no un tono, y de paso
 * no le pisan el canal de pulso a nada.
 *
 * Formato de los registros (ver nesdev):
 *   pulso  control ($4000): duty(2) | halt(1) | volumen_constante(1) | vol/envelope(4)
 *   pulso  ramp    ($4001): habilita(1) | periodo(3) | negar(1) | desplazamiento(3)
 *                           'negar' en 1 = sube el tono, en 0 = lo baja
 *   ruido  control ($400C): --(2) | halt(1) | volumen_constante(1) | vol/envelope(4)
 *   ruido  period  ($400E): modo(1) | ----(3) | indice de frecuencia(4), mas alto = mas grave
 *   len_period_high / len:  indice de duracion en los 5 bits altos
 * Los periodos de tono estan calculados para el clock de CPU de PAL
 * (~1.66MHz), que es el target de Truco86 (ver docs/ARQUITECTURA.md). */
typedef struct {
    unsigned char noise;      /* 0 = pulso 1, 1 = canal de ruido */
    unsigned char control;
    unsigned char ramp;       /* barrido; en el canal de ruido no se usa */
    unsigned char period_low;
    unsigned char len_period_high;
} SfxPreset;

static const SfxPreset PRESETS[7] = {
    /* SFX_MOVE: click corto y agudo (~1400Hz) con decaimiento rapido, duty
       25% para que sea seco y no moleste al repetirlo mucho. */
    { 0, 0x41, 0x08, 0x49, 0x48 },
    /* SFX_CONFIRM: barrido hacia ARRIBA desde ~700Hz: suena a "listo". */
    { 0, 0x86, 0x9A, 0x93, 0x68 },
    /* SFX_CARD: golpe de ruido corto = la carta cayendo sobre la mesa. */
    { 1, 0x04, 0x00, 0x0B, 0x48 },
    /* SFX_CANTO: barrido hacia ABAJO desde ~500Hz, largo y con decaimiento
       lento: es el efecto mas "hablado" del juego, tiene que destacarse. */
    { 0, 0x88, 0xA3, 0xCF, 0xA0 },
    /* SFX_WIN: barrido hacia arriba desde ~600Hz, brillante y largo. */
    { 0, 0x8A, 0xAC, 0xAC, 0xA0 },
    /* SFX_LOSE: barrido hacia abajo desde ~180Hz, grave y largo. */
    { 0, 0x8A, 0xC4, 0x40, 0xA2 },
    /* SFX_DEAL: ruido mas grave y un poco mas largo = el mazo repartiendo. */
    { 1, 0x06, 0x00, 0x0D, 0x60 },
};

void sound_init(void)
{
    APU.fcontrol = 0x00;         /* modo de 4 pasos, sin IRQ del frame counter */
    APU.status = APU_CH_SFX;     /* pulso 1 (tonos) + ruido (carta/reparto) */
}

void sound_play(SfxId id)
{
    const SfxPreset *p = &PRESETS[id];
    if (p->noise) {
        APU.noise.control = p->control;
        APU.noise.period = p->period_low;
        APU.noise.len = p->len_period_high;
    } else {
        APU.pulse[0].control = p->control;
        APU.pulse[0].ramp = p->ramp;
        APU.pulse[0].period_low = p->period_low;
        APU.pulse[0].len_period_high = p->len_period_high;
    }
}
