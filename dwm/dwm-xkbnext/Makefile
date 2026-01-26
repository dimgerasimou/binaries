CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wno-deprecated-declarations -Os
LDLIBS   ?= -lX11

PREFIX   ?= $(HOME)/.local
BINDIR   := bin

BIN      := dwm-xkbnext
SRC      := dwm-xkbnext.c
TARGET   := $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET): $(SRC) | $(BINDIR) config.h
	@echo "CC $< -> $@"
	@$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(BINDIR):
	@mkdir -p $@

config.h:
	@cp config.def.h $@

clean:
	@echo "RM $(BINDIR)"
	@rm -rf $(BINDIR)

install: $(TARGET)
	@echo "INSTALL $(BIN) -> $(DESTDIR)$(PREFIX)/bin/$(BIN)"
	@install -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	@echo "UNINSTALL $(DESTDIR)$(PREFIX)/bin/$(BIN)"
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

.PHONY: all clean install uninstall
