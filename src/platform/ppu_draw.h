#ifndef TRUCO86_PPU_DRAW_H
#define TRUCO86_PPU_DRAW_H

/* NOTA HISTORICA: la version inicial de este juego se veia como una
 * pantalla solida de un solo color en un emulador real (fceux), aunque el
 * armado del nametable era correcto. La causa: escribir una direccion de
 * VRAM por $2006 con el render prendido pisa, en el registro interno "t"
 * de la PPU, los bits de que nametable se usa — y ni $2005 (scroll) ni
 * nada que no sea un $2000 (PPU_CTRL) los repone. Ver
 * ppu_finish_vram_update() mas abajo y docs/GRAFICOS.md para el detalle
 * completo (fue diagnosticado reproduciendo el ROM real en fceux con
 * screenshots automatizados por Lua, no a simple vista). */

/* Utilidades minimas de dibujo en PPU.
 *
 * IMPORTANTE (escribir VRAM con el render prendido): ppu_set_tile(),
 * ppu_load_palette() y ppu_set_backdrop() escriben $2006/$2007, y eso es
 * seguro en cuanto a TIEMPO solo durante el render apagado o dentro de la
 * ventana de vblank (justo despues de wait_vblank()). Pero ademas, escribir
 * $2006 SIEMPRE pisa el registro interno de scroll de la PPU (aunque se
 * haga en el momento correcto): si no se restaura con ppu_reset_scroll()
 * antes de que termine el vblank, los frames siguientes se renderizan
 * desde esa direccion en vez de (0,0) y la pantalla queda en blanco o
 * glitcheada. Entonces, toda tanda de escrituras de VRAM hecha con el
 * render prendido debe terminar con un ppu_finish_vram_update(). Antes de
 * prender el render por primera vez no hace falta nada de esto.
 *
 * OJO: no alcanza con restaurar el scroll ($2005) solo. El $2006 de
 * direccion tambien pisa, dentro del registro interno "t" de la PPU, los
 * bits de que nametable se usa — y esos bits los pone unicamente $2000
 * (PPU_CTRL), $2005 no los toca. Por eso ppu_finish_vram_update() escribe
 * scroll Y ADEMAS reescribe PPU_CTRL. */

/* Pone en 0 (tile 0 = transparente) los 960 bytes del nametable 0 y en 0
 * (paleta de fondo 0) los 64 bytes de su tabla de atributos. */
void ppu_clear_nametable0(void);

/* Carga las 32 entradas de paleta (4 de fondo + 4 de sprites, 8 colores
 * cada grupo de 4 paletas) en $3F00-$3F1F. */
void ppu_load_palette(const unsigned char pal[32]);

/* Escribe un tile en el nametable 0, en la columna/fila dada (0..31 / 0..29). */
void ppu_set_tile(unsigned char col, unsigned char row, unsigned char tile);

/* Asigna una de las 4 paletas de fondo (0..3, ver ppu_load_palette()) al
 * bloque de 2x2 tiles al que pertenece (col,row): la tabla de atributos de
 * la PPU solo puede elegir paleta de a bloques de 2x2 tiles (16x16 px), no
 * tile por tile, asi que cualquier otro tile del mismo bloque (col/row
 * pueden caer en cualquiera de los 4 tiles del bloque, no hace falta que
 * sean pares) queda con la misma paleta. Mantiene una copia en RAM de la
 * tabla de atributos (64 bytes) para poder hacer el leer-modificar-escribir
 * sin tener que leer la VRAM real (mas lento y necesita el truco del
 * buffer de lectura de $2007). Como cualquier otra escritura de VRAM, hay
 * que llamarla con el render apagado o recien despues de wait_vblank(). */
void ppu_set_palette_block(unsigned char col, unsigned char row, unsigned char palette);

/* Pone los 64 sprites de la OAM fuera de pantalla (Y=$FF). Hay que llamarla
 * antes de prender el render la primera vez: la OAM arranca con contenido
 * indeterminado y, si no se limpia, pueden verse sprites basura. */
void ppu_clear_oam(void);

/* Escribe (0,0) en el registro de scroll ($2005), dos veces (X e Y). Es
 * obligatorio hacerlo antes de prender el render: si nunca se escribe
 * scroll, la ventana visible puede quedar desalineada del nametable. */
void ppu_reset_scroll(void);

/* Restaura scroll (0,0) Y reescribe PPU_CTRL con el valor dado (para
 * recuperar los bits de nametable de "t", ver la nota de arriba). Llamarla
 * justo despues de cualquier tanda de escrituras a $2006/$2007 hecha con
 * el render prendido. */
void ppu_finish_vram_update(unsigned char ctrl);

/* Cambia el color universal de fondo ($3F00). Ver la nota de arriba: solo
 * llamarla justo despues de wait_vblank(), y seguirla con
 * ppu_finish_vram_update() si el render ya esta prendido. */
void ppu_set_backdrop(unsigned char color);

#endif
