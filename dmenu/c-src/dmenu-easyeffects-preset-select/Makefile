# Simple equalizer preset selector using dmenu 

CC      := cc
CFLAGS  := -Wall -Wextra -Werror -Wno-deprecated-declarations -Os
PREFIX  := $(HOME)/.local/bin

SRC := dmenu-easyeffects-preset-select.c

all: dmenu-easyeffects-preset-select

options:
	@echo "dmenu-easyeffects-preset-select build options:"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "CC      = $(CC)"

dmenu-easyeffects-preset-select:
	@echo "making dmenu-easyeffects-preset-select"
	${CC} -o $@ $(SRC) $(CFLAGS)

clean:
	@rm -rf dmenu-easyeffects-preset-select

install: all
	@mkdir -p $(PREFIX)
	@cp dmenu-easyeffects-preset-select $(PREFIX)

uninstall:
	@rm $(PREFIX)/dmenu-easyeffects-preset-select

.PHONY: all options clean
