CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wno-deprecated-declarations -Os
CPPFLAGS ?=
LDLIBS   ?= -lpulse

PREFIX   ?= $(HOME)/.local
BINDIR   := bin

BIN      := dwm-audio
SRCDIR   := src
SRCS     := $(SRCDIR)/main.c $(SRCDIR)/pa.c
INCS     := -I.
TARGET   := $(BINDIR)/$(BIN)

PKG      :=
CPPFLAGS += $(shell pkg-config --cflags $(PKG) 2>/dev/null)
LDLIBS   += $(shell pkg-config --libs   $(PKG) 2>/dev/null)

all: $(TARGET)

$(TARGET): $(SRCS) $(SRCDIR)/pa.h | $(BINDIR) config.h
	@echo "CC $(SRCS) -> $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(INCS) -o $@ $(SRCS) $(LDFLAGS) $(LDLIBS)

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
