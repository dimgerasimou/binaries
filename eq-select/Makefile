# eq-select - select easyeffects preset

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-deprecated-declarations -Os
LDFLAGS ?= -s
LDLIBS  ?=

PREFIX  ?= $(HOME)/.local
BINDIR  ?= bin

SRC     = main.c
BIN     = eq-select
TARGET  = $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET): $(SRC) config.h | $(BINDIR)
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
