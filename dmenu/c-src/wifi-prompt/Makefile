# Wifi prompt with dmenu

CC      := cc
CFLAGS  := -Wall -Wextra -Werror -Wno-deprecated-declarations -O0 -g
LDFLAGS := -lm `pkg-config --cflags --libs libnm`

all: options dmenu-wifi-prompt

options:
	@echo "dmenu-wifi-prompt build options:"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

dmenu-wifi-prompt:
	@echo "making dmenu-wifi-prompt"
	@mkdir -p bin
	@${CC} -o bin/dmenu-wifi-prompt main.c ${CFLAGS} ${LDFLAGS}

clean:
	@rm -rf bin

.PHONY: all options clean