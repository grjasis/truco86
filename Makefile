CC65_TARGET := nes
ROM          := build/truco86.nes
CFG          := nes.cfg
CHR          := res/chr/cards.chr
SRCS         := src/main.c src/crt0.s src/chr_bank.s \
                 src/game/deck.c src/game/card_rank.c src/game/match.c \
                 src/game/envido.c src/game/flor.c src/game/canto.c src/game/ai.c \
                 src/platform/ppu_draw.c src/platform/cards_render.c \
                 src/platform/vsync.c src/platform/input.c src/platform/text.c \
                 src/platform/canto_ui.c src/platform/truco_ui.c src/platform/sound.c \
                 src/platform/music.c

.PHONY: all clean run test chr web

all: $(ROM)

# Copia el .nes compilado a web/ (pagina estatica con jsnes, ver web/README.md)
web: $(ROM)
	cp $(ROM) web/truco86.nes

build:
	mkdir -p build

chr: $(CHR)

$(CHR): tools/make_chr.py
	python3 tools/make_chr.py $(CHR)

$(ROM): $(SRCS) $(CFG) $(CHR) | build
	cl65 -t $(CC65_TARGET) -O -C $(CFG) -o $(ROM) $(SRCS)
	python3 tools/set_pal_header.py $(ROM)

test:
	$(MAKE) -C src/test

clean:
	rm -rf build src/test/build

run: $(ROM)
	@echo "Abrir $(ROM) en un emulador con soporte de region PAL/PAL-N (ej: Mesen)."
