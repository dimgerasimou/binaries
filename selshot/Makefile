CC       ?= cc
CFLAGS   ?= -std=c99 -Wall -Wextra -Os
CPPFLAGS ?=
LDLIBS   ?=

PREFIX   ?= $(HOME)/.local
BINDIR   := bin

BIN      := selshot
SRC      := $(BIN).c
TARGET   := $(BINDIR)/$(BIN)

PKG      := libnotify
ifneq ($(shell pkg-config --exists $(PKG) && echo ok),ok)
$(error $(PKG) not found - install its development package)
endif
CPPFLAGS += $(shell pkg-config --cflags $(PKG))
LDLIBS   += $(shell pkg-config --libs   $(PKG))

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
