#include <nes.h>
#include "music.h"
#include "sound.h"
#include "vsync.h"

/* --- "La Cumparsita", parte A (16 compases de 2/4) ---
 *
 * Melodia transcripta nota por nota de un arreglo en SOL MENOR del tango
 * (el mismo que circula como partitura para flauta), no "de oido": cada
 * evento de abajo sale de las notas y duraciones reales de esa parte A, con
 * los silencios del staccato incluidos. Antes habia una melodia que decia
 * ser esta pero era otra cosa (una transcripcion RTTTL que no coincide con
 * el tango).
 *
 * Duraciones: la corchea son 12 cuadros a 50 cuadros/seg (PAL-N), o sea
 * negra = 24 cuadros ~ 125 pulsos por minuto, que es un tempo de tango
 * razonable. La parte A entera son 768 cuadros (~15 s) y se repite en loop.
 *
 * Va por el PULSO 2; el pulso 1 queda libre para los SFX (sound.c) y el
 * TRIANGULO lleva el bajo (ver BASS mas abajo). */

/* Periodos para el clock de CPU de PAL (~1.66MHz, ver docs/ARQUITECTURA.md):
 * period = round(1662607 / (16 * frecuencia)) - 1. El indice 0 es silencio. */
static const unsigned int NOTE_PERIOD[] = {
       0, /* silencio */
     280, /* Fa#4 */
     264, /* Sol4 */
     235, /* La4  */
     222, /* Sib4 */
     198, /* Do5  */
     186, /* Do#5 */
     176, /* Re5  */
     166, /* Mib5 */
     139, /* Fa#5 */
     132, /* Sol5 */
     117, /* La5  */
     110, /* Sib5 */
      98, /* Do6  */
      87, /* Re6  */
};

typedef struct {
    unsigned char pitch;  /* indice en NOTE_PERIOD, 0 = silencio */
    unsigned char frames; /* duracion a 50 cuadros/seg (PAL-N) */
} Note;

static const Note MELODY[] = {
    {  7,  6 }, {  0,  6 }, { 13,  6 }, {  0,  6 }, { 11,  6 },
    {  0,  6 }, {  9,  6 }, {  0, 12 }, {  7,  6 }, {  8,  6 },
    {  7,  6 }, {  6, 12 }, {  7, 12 }, {  7,  6 }, {  0,  6 },
    { 14,  6 }, {  0,  6 }, { 12,  6 }, {  0,  6 }, { 10,  6 },
    {  0, 12 }, {  7,  6 }, {  8,  6 }, {  7,  6 }, {  6, 12 },
    {  7, 12 }, {  7,  6 }, {  0,  6 }, { 13,  6 }, {  0,  6 },
    { 11,  6 }, {  0,  6 }, {  9,  6 }, {  0, 12 }, {  7,  6 },
    {  8,  6 }, {  7,  6 }, {  6, 12 }, {  7, 12 }, {  7,  6 },
    {  0,  6 }, { 14,  6 }, {  0,  6 }, { 12,  6 }, {  0,  6 },
    { 10,  6 }, {  0, 12 }, {  7,  6 }, {  8,  6 }, {  7,  6 },
    {  6, 12 }, {  7, 12 }, {  5, 12 }, { 10, 12 }, {  9, 12 },
    { 10, 12 }, {  0,  6 }, {  9,  6 }, { 10,  6 }, {  9,  6 },
    { 10, 12 }, {  9, 12 }, {  4, 12 }, { 10, 12 }, {  9, 12 },
    { 10, 12 }, {  0,  6 }, {  9,  6 }, { 10,  6 }, {  9,  6 },
    { 10, 12 }, {  9, 12 }, {  3, 12 }, {  7, 12 }, {  5, 12 },
    {  7, 12 }, {  0,  6 }, {  5,  6 }, {  4,  6 }, {  3,  6 },
    {  2, 12 }, {  1, 12 }, {  2, 18 }, {  8,  6 }, {  7,  6 },
    {  5, 12 }, {  4,  3 }, {  3,  3 }, {  2, 24 }, { 10, 24 },
};
#define MELODY_LEN (sizeof(MELODY) / sizeof(MELODY[0]))

/* control del pulso 2 ($4004): duty 50% | halt del contador de longitud (la
 * duracion la maneja music_update() a mano) | volumen constante | volumen. */
#define MEL_CTRL      0xB9
#define MEL_CTRL_MUTE 0xB0

/* --- Bajo de tango (triangulo) ---
 * Marcato de negras: fundamental en el primer tiempo del compas y quinta en
 * el segundo, sobre la armonia de la parte A (Re7 dominante / Sol menor, con
 * un Do menor en el anteultimo compas). Son 32 negras = 16 compases = los
 * mismos 768 cuadros que la melodia, asi que las dos vueltas quedan siempre
 * sincronizadas. */
static const unsigned int BASS_PERIOD[] = {
     707, /* 0: Re2  */
     471, /* 1: La2  */
     529, /* 2: Sol2 */
     353, /* 3: Re3  */
     793, /* 4: Do2  */
};

static const unsigned char BASS[] = {
    0, 1,  0, 1,  2, 3,  0, 1,   /* Re7 | Re7 | Solm | Re7 */
    0, 1,  0, 1,  2, 3,  0, 1,   /* idem (se repiten los 4 compases) */
    0, 1,  0, 1,  2, 3,  2, 3,   /* Re7 | Re7 | Solm | Solm */
    0, 1,  2, 3,  4, 2,  2, 3,   /* Re7 | Solm | Dom | Solm */
};
#define BASS_LEN    (sizeof(BASS) / sizeof(BASS[0]))
#define BEAT_FRAMES 24 /* negra */
#define BASS_GATE    8 /* cuadros de silencio al final de cada negra (marcato) */

/* $4008: bit7 (control) prendido = el contador lineal se recarga siempre, o
 * sea que la nota suena hasta que la cortemos nosotros. Escribir 0x00 lo
 * apaga y el canal se calla enseguida. */
#define TRI_ON  0x81
#define TRI_OFF 0x00

static unsigned char mel_idx;
static unsigned char mel_left;
static unsigned char bass_idx;
static unsigned char bass_left;

static void mel_start(void)
{
    unsigned int period = NOTE_PERIOD[MELODY[mel_idx].pitch];
    mel_left = MELODY[mel_idx].frames;
    if (MELODY[mel_idx].pitch == 0) {
        APU.pulse[1].control = MEL_CTRL_MUTE;
        return;
    }
    /* escribir len_period_high de ultimo relanza la nota (fase + contador),
       igual que en sound.c: por eso el orden de los campos importa. */
    APU.pulse[1].control = MEL_CTRL;
    APU.pulse[1].ramp = 0x08; /* sin barrido */
    APU.pulse[1].period_low = (unsigned char)period;
    APU.pulse[1].len_period_high = (unsigned char)(period >> 8);
}

static void bass_start(void)
{
    unsigned int period = BASS_PERIOD[BASS[bass_idx]];
    bass_left = BEAT_FRAMES;
    APU.triangle.counter = TRI_ON;
    APU.triangle.period_low = (unsigned char)period;
    APU.triangle.len_period_high = (unsigned char)(period >> 8);
}

void music_init(void)
{
    /* $4015 (APU.status) tiene lectura y escritura con significados distintos
       en el hardware real (leer da flags de actividad/IRQ, no lo que se
       escribio): un "leer-modificar-escribir" (|=, &=) corrompe los canales
       habilitados. Por eso se escribe el valor completo de una, sin leer. */
    APU.status = APU_CH_MUSIC;
    mel_idx = 0;
    bass_idx = 0;
    mel_start();
    bass_start();
}

void music_update(void)
{
    /* Un cuadro de silencio antes de cada nota nueva: sin eso las notas
       pegadas (las corcheas repetidas del tango) se escuchan como una sola
       nota larga, porque el volumen es constante y no hay ataque. */
    if (mel_left == 2) APU.pulse[1].control = MEL_CTRL_MUTE;
    if (--mel_left == 0) {
        ++mel_idx;
        if (mel_idx >= MELODY_LEN) mel_idx = 0;
        mel_start();
    }

    if (bass_left == BASS_GATE) APU.triangle.counter = TRI_OFF;
    if (--bass_left == 0) {
        ++bass_idx;
        if (bass_idx >= BASS_LEN) bass_idx = 0;
        bass_start();
    }
}

void music_stop(void)
{
    APU.pulse[1].control = MEL_CTRL_MUTE;
    APU.triangle.counter = TRI_OFF;
    APU.status = APU_CH_SFX; /* deja los canales de SFX; ver nota en music_init() */
}

/* --- Jingles de fin de mano --- */

static void play_jingle_note(unsigned char pitch, unsigned char frames)
{
    unsigned int period = NOTE_PERIOD[pitch];
    APU.pulse[1].control = 0xBA;
    APU.pulse[1].ramp = 0x08;
    APU.pulse[1].period_low = (unsigned char)period;
    APU.pulse[1].len_period_high = (unsigned char)(period >> 8);
    while (frames--) {
        wait_vblank();
    }
}

/* Arpegio de Sol menor (la tonalidad del tango de la pantalla de titulo)
 * ascendente para ganar y descendente para perder: Sol-Sib-Re-Sol. */
void music_play_win(void)
{
    APU.status = APU_CH_MUSIC;
    play_jingle_note(2, 10);   /* Sol4 */
    play_jingle_note(4, 10);   /* Sib4 */
    play_jingle_note(7, 10);   /* Re5  */
    play_jingle_note(10, 45);  /* Sol5 */
    APU.pulse[1].control = MEL_CTRL_MUTE;
    APU.status = APU_CH_SFX;
}

void music_play_lose(void)
{
    APU.status = APU_CH_MUSIC;
    play_jingle_note(10, 10);  /* Sol5 */
    play_jingle_note(7, 10);   /* Re5  */
    play_jingle_note(4, 10);   /* Sib4 */
    play_jingle_note(2, 45);   /* Sol4 */
    APU.pulse[1].control = MEL_CTRL_MUTE;
    APU.status = APU_CH_SFX;
}
