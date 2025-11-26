# dmenu-equalizer-select - select easyeffects preset
PREFIX = $(HOME)/.local/bin

CC = cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os
LDFLAGS = -s

SRC = dmenu-equalizer-select.c
OBJ = ${SRC:.c=.o}

all: dmenu-equalizer-select

config.h:
	cp config.def.h config.h

.c.o:
	${CC} -c ${CFLAGS} $<

dmenu-equalizer-select: config.h ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dmenu-equalizer-select ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dmenu-equalizer-select ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu-equalizer-select

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dmenu-equalizer-select

.PHONY: all clean install uninstall
