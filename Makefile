.PHONY: all clean dist app

DIST_DIR := dist
SRC_DIR := src
INCLUDE_DIR := $(SRC_DIR)/include
SRC := $(wildcard $(SRC_DIR)/*.c)
BIN := $(DIST_DIR)/SeeWin

CFLAGS := -Wall -Wextra -std=c99 -I$(INCLUDE_DIR) -framework Cocoa -framework Carbon

all: $(BIN)

$(BIN): $(SRC) | $(DIST_DIR)
	clang $(CFLAGS) -o $@ $^

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

app: $(BIN)
	./appify -s $(BIN) -n $(DIST_DIR)/SeeWinApp

clean:
	rm -rf $(DIST_DIR)

