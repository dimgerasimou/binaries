# dwmblocks status bar executable blocks


PREFIX    := ${HOME}/.local/bin/dwmblocks
CC        := cc
CFLAGS    := -Wall -Wextra -Werror -Wno-deprecated-declarations -Os
LFLAGS    := -lm -lX11 -lxkbfile `pkg-config --cflags --libs libnotify libnm`


BIN = time mediacontrol bluetooth-menu keyboard battery date kernel bluetooth internet memory power volume
BIN_PATH = $(addprefix $(PREFIX)/, $(BIN))


all: $(BIN)

$(BIN): %: src/%.c src/common.c include/colorscheme.h
	@mkdir -p bin
	@$(CC) -o bin/$@ $< src/common.c ${CFLAGS} ${LFLAGS}

include/colorscheme.h: src/loadresources.c
	@echo "Making colorscheme.h"
	@$(CC) -o loadresources $< ${CFLAGS} -lX11
	@./loadresources
	@rm -f loadresources

clean:
	@echo "cleaning status-bar-exec directory"
	@rm -rf bin

install: all
	@echo "Installing dwmblocks status bar blocks"
	@mkdir -p $(PREFIX)
	@cp bin/* $(PREFIX)/

uninstall:
	@echo "Uninstalling dwmblocks status bar blocks"
	@rm $(BIN_PATH)

.PHONY: all clean