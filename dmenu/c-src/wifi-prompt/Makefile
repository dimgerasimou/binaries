# Wifi prompt with dmenu

CC      := cc
CFLAGS  := -Wall -Wextra  -Wno-deprecated-declarations -Os
LDFLAGS := -lm `pkg-config --cflags --libs libnm libnotify`

SRC := main.c input.c output.c
OBJ := ${SRC:.c=.o}

all: wifi-prompt

options:
	@echo "wifi-prompt build options:"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} ${LDFLAGS} $<

${OBJ}: input.h output.h

wifi-prompt: ${OBJ}
	@echo "making wifi-prompt"
	@mkdir -p bin
	${CC} -o bin/$@ ${OBJ} ${LDFLAGS}

clean:
	@rm -rf bin
	@rm -f ${OBJ}

.PHONY: all options clean