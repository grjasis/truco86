CC     := cc
CFLAGS := -std=c99 -Wall -Wextra -O1
SRCS   := test_rules.c ../game/deck.c ../game/card_rank.c ../game/envido.c ../game/flor.c ../game/match.c ../game/canto.c ../game/ai.c
BIN    := build/test_rules

.PHONY: test clean

test: $(BIN)
	./$(BIN)

$(BIN): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(BIN) $(SRCS)

clean:
	rm -rf build
