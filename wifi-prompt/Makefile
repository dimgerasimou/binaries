CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-deprecated-declarations -Os
LDFLAGS ?= -lm $(shell pkg-config --cflags --libs libnm libnotify)
LDLIBS  ?=

PREFIX  ?= $(HOME)/.local
BINDIR  ?= bin

SRC     = main.c input.c output.c
BIN     = wifi-prompt
TARGET  = $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET): $(SRC) input.h output.h config.h | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

config.h:
	cp config.def.h config.h

$(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BINDIR)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

.PHONY: all clean install uninstall
