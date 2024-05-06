# Wifi prompt with dmenu

CC      := cc
CFLAGS  := -Wall -Wextra -Werror -Wno-deprecated-declarations -Os
LDFLAGS := -lm `pkg-config --cflags --libs libnm`

all: options wifi-prompt

options:
	@echo "wifi-prompt build options:"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

wifi-prompt:
	@echo "making wifi-prompt"
	@mkdir -p bin
	@${CC} -o bin/wifi-prompt main.c ${CFLAGS} ${LDFLAGS}

clean:
	@rm -rf bin

.PHONY: all options clean