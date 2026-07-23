; Truco86 - arranque minimo propio para NES (reemplaza el crt0.o de nes.lib).
;
; El crt0.o que trae cc65 para el target "nes" incluye, pegado en el mismo
; objeto, una fuente de texto por defecto de 4KB en el segmento CHARS. Como
; ese objeto se linkea completo o nada, no hay forma de aprovechar el resto
; del runtime sin arrastrar esos 4KB, que no dejan lugar para nuestro propio
; CHR de cartas en un banco de 8KB (mapper NROM). Por eso este proyecto trae
; su propio crt0: header iNES, vectores, inicializacion de RAM/stack y un
; NMI minimo que solo cuenta frames (usado por platform/vsync.c).
;
; El resto del runtime de cc65 (rutinas de soporte del compilador, driver de
; joystick, etc.) se sigue linkeando normalmente desde nes.lib.

.import _main
.importzp sp, ptr1, ptr2
.import __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__
.import __PARAMSTACK_TOP__

.export __STARTUP__ : absolute = 1     ; fuerza a cl65/ld65 a no buscar crt0.o en nes.lib
.export _exit
.export _nes_frame_count

.segment "HEADER"
        .byte   "NES", $1A     ; firma iNES
        .byte   2               ; 2 bancos de 16KB de PRG-ROM (32KB)
        .byte   1               ; 1 banco de 8KB de CHR-ROM
        .byte   $01             ; flags6: mirroring vertical, mapper 0 (NROM)
        .byte   $00             ; flags7: mapper 0, sin flags extra
        .byte   0,0,0,0,0,0,0,0 ; bytes 8-15 sin uso (iNES 1.0)

.segment "BSS"
_nes_frame_count: .res 1        ; incrementado en cada NMI, ver platform/vsync.c

.segment "STARTUP"

Reset:
        sei
        cld
        ldx     #$40
        stx     $4017           ; deshabilita el IRQ del frame counter de la APU
        ldx     #$FF
        txs                     ; stack real del 6502
        inx                     ; x = 0
        stx     $2000           ; PPU_CTRL = 0 (NMI off)
        stx     $2001           ; PPU_MASK = 0 (render off)
        stx     $4010           ; deshabilita IRQ de la DMC

@vblank1:
        bit     $2002
        bpl     @vblank1

        ; limpia $0000-$07FF (zeropage, pila 6502, buffers bajos)
        txa
@clearlow:
        sta     $0000,x
        sta     $0100,x
        sta     $0200,x
        sta     $0300,x
        sta     $0400,x
        sta     $0500,x
        sta     $0600,x
        sta     $0700,x
        inx
        bne     @clearlow

        ; limpia $6000-$7FFF (banco de RAM de 8KB usado para DATA/BSS)
        lda     #$00
        sta     ptr1
        lda     #$60
        sta     ptr1+1
        ldx     #$20            ; 32 paginas de 256 bytes = 8KB
        ldy     #$00
@clearsram_page:
        tya
@clearsram_byte:
        sta     (ptr1),y
        iny
        bne     @clearsram_byte
        inc     ptr1+1
        dex
        bne     @clearsram_page

@vblank2:
        bit     $2002
        bpl     @vblank2

        ; copia el segmento DATA (globales inicializadas) de ROM a RAM
        lda     #<__DATA_LOAD__
        sta     ptr1
        lda     #>__DATA_LOAD__
        sta     ptr1+1
        lda     #<__DATA_RUN__
        sta     ptr2
        lda     #>__DATA_RUN__
        sta     ptr2+1

        ldy     #$00
        lda     #>__DATA_SIZE__ ; cantidad de paginas completas
        beq     @copy_tail
        tax
@copy_page:
        lda     (ptr1),y
        sta     (ptr2),y
        iny
        bne     @copy_page
        inc     ptr1+1
        inc     ptr2+1
        dex
        bne     @copy_page

@copy_tail:
        ldx     #<__DATA_SIZE__ ; bytes sueltos que sobran (< 256)
        beq     @copy_done
@copy_tail_loop:
        lda     (ptr1),y
        sta     (ptr2),y
        iny
        dex
        bne     @copy_tail_loop
@copy_done:

        ; pila de parametros de cc65 (software stack usado por el codigo C generado)
        lda     #<__PARAMSTACK_TOP__
        sta     sp
        lda     #>__PARAMSTACK_TOP__
        sta     sp+1

        jsr     _main

_exit:
        jmp     _exit           ; main() no deberia retornar nunca

; ---------------------------------------------------------------------------
NMI:
        pha
        txa
        pha
        tya
        pha

        inc     _nes_frame_count

        pla
        tay
        pla
        tax
        pla
        rti

IRQ:
        rti                     ; no se usa IRQ de mapper en NROM

.segment "VECTORS"
        .word   NMI
        .word   Reset
        .word   IRQ
