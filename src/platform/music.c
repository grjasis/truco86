#include <nes.h>
#include "music.h"
#include "vsync.h"

/* Periodos calculados para el clock de CPU de PAL (~1.66MHz, ver
 * docs/ARQUITECTURA.md): period = round(1662607 / (16*frecuencia)) - 1.
 * Bajadas una octava respecto de la transcripcion RTTTL de abajo, para
 * que sea un tono mas comodo en el canal de pulso del NES. */
#define NOTE_GS3_LOW 0xF3 /* Sol#3 / Lab3 */
#define NOTE_GS3_HIGH 0x01
#define NOTE_A3_LOW  0xD7
#define NOTE_A3_HIGH 0x01
#define NOTE_B3_LOW  0xA4
#define NOTE_B3_HIGH 0x01
#define NOTE_CS4_LOW 0x76 /* Do#4 / Reb4 */
#define NOTE_CS4_HIGH 0x01
#define NOTE_D4_LOW  0x61
#define NOTE_D4_HIGH 0x01
#define NOTE_E4_LOW  0x3A
#define NOTE_E4_HIGH 0x01

/* control ($4004): duty 50% (10) | halt/loop del envelope (1, para que no
 * decaiga solo: la duracion de cada nota la maneja music_update() a mano,
 * no el contador de longitud de la APU) | volumen constante (1) | volumen. */
#define CTRL(vol) (unsigned char)(0xB0 | (vol))
#define CTRL_REST CTRL(0)

typedef struct {
    unsigned char period_low;
    unsigned char period_high; /* bits 0-2 del periodo (bits altos) */
    unsigned char ctrl;
    unsigned char frames; /* duracion a 50 cuadros/seg (PAL-N) */
} Note;

#define EIGHTH 12 /* corchea, a ~50 cuadros/seg (PAL-N) */
#define REST   12

/* Frase de "La Cumparsita" (ver la nota larga en music.h), transcripta de
 * la notacion RTTTL del ringtone clasico de Nokia (todas corcheas: "8" en
 * esa notacion), bajada una octava. Notas (en la octava original del
 * RTTTL): 8c#1 8c#1 8d1 8c#1 8d1 8c#1 8d1 8c#1 8c#1 8d1 8c#1 8e1 8d1 8c#1
 * 8b0 8a0 8a0 8b0 8c#1 8d1 8c#1 8b0 8a0 8g#0 8a0 8b0 8a0. */
static const Note MELODY[] = {
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_E4_LOW,  NOTE_E4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_B3_LOW,  NOTE_B3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_A3_LOW,  NOTE_A3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_A3_LOW,  NOTE_A3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_B3_LOW,  NOTE_B3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_D4_LOW,  NOTE_D4_HIGH,  CTRL(9), EIGHTH },
    { NOTE_CS4_LOW, NOTE_CS4_HIGH, CTRL(9), EIGHTH },
    { NOTE_B3_LOW,  NOTE_B3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_A3_LOW,  NOTE_A3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_GS3_LOW, NOTE_GS3_HIGH, CTRL(9), EIGHTH },
    { NOTE_A3_LOW,  NOTE_A3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_B3_LOW,  NOTE_B3_HIGH,  CTRL(9), EIGHTH },
    { NOTE_A3_LOW,  NOTE_A3_HIGH,  CTRL(9), (unsigned char)(EIGHTH * 2) },
    { 0,            0,             CTRL_REST, REST },
};
#define MELODY_LEN (sizeof(MELODY) / sizeof(MELODY[0]))

static unsigned char note_idx;
static unsigned char frames_left;

static void play_note(unsigned char i)
{
    /* escribir len_period_high de ultimo relanza la nota (fase +
       contador de duracion), igual que sound.c; como 'ctrl' tiene el bit
       de halt prendido, el contador de duracion de la APU nunca corta la
       nota sola: la corta music_update() cuando toca la siguiente. */
    APU.pulse[1].control = MELODY[i].ctrl;
    APU.pulse[1].ramp = 0x00;
    APU.pulse[1].period_low = MELODY[i].period_low;
    APU.pulse[1].len_period_high = MELODY[i].period_high;
    frames_left = MELODY[i].frames;
}

void music_init(void)
{
    /* $4015 (APU.status) tiene lectura y escritura con significados
       distintos en el hardware real (leer da flags de actividad/IRQ, no
       lo que se escribio): un "leer-modificar-escribir" (|=, &=) corrompe
       los canales habilitados. Por eso se escribe el valor completo de
       una, sin leer: bit0 pulso1 (SFX, sound.c) + bit1 pulso2 (musica). */
    APU.status = 0x03;
    note_idx = 0;
    play_note(note_idx);
}

void music_update(void)
{
    if (frames_left == 0) {
        note_idx = (unsigned char)(note_idx + 1);
        if (note_idx >= MELODY_LEN) note_idx = 0;
        play_note(note_idx);
    } else {
        --frames_left;
    }
}

void music_stop(void)
{
    APU.pulse[1].control = CTRL_REST;
    APU.status = 0x01; /* deja solo el pulso 1 (SFX, ver sound_init()); ver nota en music_init() */
}

static void play_jingle_note(unsigned char period_low, unsigned char period_high, unsigned char frames)
{
    APU.pulse[1].control = CTRL(10);
    APU.pulse[1].ramp = 0x00;
    APU.pulse[1].period_low = period_low;
    APU.pulse[1].len_period_high = period_high;
    while (frames--) {
        wait_vblank();
    }
}

/* Arpegio de La mayor ascendente (La-Do#-Mi, triunfal) o descendente
 * (Mi-Do#-La, para la derrota): mismas 3 notas de "La Cumparsita" de
 * arriba, asi que no hacen falta periodos nuevos. */
void music_play_win(void)
{
    APU.status = 0x03;
    play_jingle_note(NOTE_A3_LOW, NOTE_A3_HIGH, 15);
    play_jingle_note(NOTE_CS4_LOW, NOTE_CS4_HIGH, 15);
    play_jingle_note(NOTE_E4_LOW, NOTE_E4_HIGH, 45);
    APU.pulse[1].control = CTRL_REST;
    APU.status = 0x01;
}

void music_play_lose(void)
{
    APU.status = 0x03;
    play_jingle_note(NOTE_E4_LOW, NOTE_E4_HIGH, 15);
    play_jingle_note(NOTE_CS4_LOW, NOTE_CS4_HIGH, 15);
    play_jingle_note(NOTE_A3_LOW, NOTE_A3_HIGH, 45);
    APU.pulse[1].control = CTRL_REST;
    APU.status = 0x01;
}
