# dwmblocks status bar executable blocks

CC        := cc
CFLAGS    := -Wall -Wextra -Werror -Wno-deprecated-declarations -Os
LFLAGS    := -lm -lX11 -lxkbfile `pkg-config --cflags --libs libnotify libnm`

BIN = time spacer mediacontrol bluetooth-menu keyboard battery date kernel bluetooth internet memory power volume

all: $(BIN)

$(BIN): %: src/%.c src/common.c include/colorscheme.h
	@mkdir -p bin
	@$(CC) -o bin/$@ $< src/common.c ${CFLAGS} ${LFLAGS}

include/colorscheme.h:
	@echo "Making colorscheme.h"
	@cc -o loadresources src/loadresources.c ${CFLAGS} ${LFLAGS}
	@./loadresources
	@rm -f loadresources

clean:
	@echo "cleaning status-bar-exec directory"
	@rm -rf bin

.PHONY: all clean