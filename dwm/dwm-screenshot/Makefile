CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wno-deprecated-declarations -Os
CPPFLAGS ?=
LDLIBS   ?=

PREFIX   ?= $(HOME)/.local
BINDIR   := bin

BIN      := dwm-screenshot
SRC      := $(BIN).c
TARGET   := $(BINDIR)/$(BIN)

PKG      := libnotify
CPPFLAGS += $(shell pkg-config --cflags $(PKG) 2>/dev/null)
LDLIBS   += $(shell pkg-config --libs   $(PKG) 2>/dev/null)

all: $(TARGET)

$(TARGET): $(SRC) | $(BINDIR)
	@echo "CC $< -> $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(BINDIR):
	@mkdir -p $@

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
