.PHONY: all clean dist app install uninstall

DIST_DIR := dist
SRC_DIR := src
INCLUDE_DIR := $(SRC_DIR)/include
SRC := $(wildcard $(SRC_DIR)/*.c)
BIN := $(DIST_DIR)/SeeWin

CFLAGS := -Wall -Wextra -std=c99 -I$(INCLUDE_DIR) -framework Cocoa -framework Carbon
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
INSTALL_NAME := SeeWin

# Output control
V ?= 0
ifeq ($(V),1)
    Q :=
    ECHO := @true
else
    Q := @
    ECHO := echo
endif

# Colors
BOLD := \033[1m
RESET := \033[0m
GREEN := \033[32m
YELLOW := \033[33m
BLUE := \033[34m

all: $(BIN)

$(BIN): $(SRC) | $(DIST_DIR)
	$(Q)$(ECHO) "$(BLUE)Compiling $(RESET)$(BOLD)$@$(RESET)"
	$(Q)clang $(CFLAGS) -o $@ $^ && \
	$(ECHO) "$(GREEN)✓ Build successful: $(RESET)$@" || \
  $(ECHO) "$(RED)✗ Build failed$(RESET)"

$(DIST_DIR):
	$(Q)mkdir -p $(DIST_DIR)

install: $(BIN)
	$(Q)$(ECHO) "$(BLUE)Installing to $(RESET)$(BOLD)$(BINDIR)/$(INSTALL_NAME)$(RESET)"
	$(Q)install -d $(BINDIR)
	$(Q)install -m 755 $(BIN) $(BINDIR)/$(INSTALL_NAME) && \
	$(ECHO) "$(GREEN)✓ Installed successfully to $(RESET)$(BINDIR)/$(INSTALL_NAME)" || \
  $(ECHO) "$(RED)✗ Install failed$(RESET)"

uninstall:
	$(Q)$(ECHO) "$(YELLOW)Uninstalling from $(RESET)$(BINDIR)/$(INSTALL_NAME)"
	$(Q)rm -f $(BINDIR)/$(INSTALL_NAME)

clean:
	$(Q)$(ECHO) "$(YELLOW)Cleaning build artifacts$(RESET)"
	$(Q)rm -rf $(DIST_DIR)

