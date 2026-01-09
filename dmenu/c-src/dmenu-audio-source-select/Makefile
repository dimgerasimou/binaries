CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-deprecated-declarations -Os
LDFLAGS ?=
LDLIBS  ?=

PREFIX  ?= $(HOME)/.local
BINDIR  ?= bin

SRC     = main.c
BIN     = dmenu-audio-source-select
TARGET  = $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET): $(SRC) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

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
