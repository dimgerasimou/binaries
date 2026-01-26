CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wno-deprecated-declarations -Os
LDLIBS   ?= -lpulse

PREFIX   ?= $(HOME)/.local
BINDIR   := bin
OBJDIR   := obj

BIN      := dwm-audio
SRCS     := dwm-audio.c audio.c
OBJS     := $(SRCS:%.c=$(OBJDIR)/%.o)
TARGET   := $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	@echo "LD $(OBJS) -> $@"
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJDIR)/%.o: %.c audio.h config.h | $(OBJDIR)
	@echo "CC $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR) $(OBJDIR):
	@mkdir -p $@

config.h:
	@cp config.def.h $@

clean:
	@echo "RM $(BINDIR) $(OBJDIR)"
	@rm -rf $(BINDIR) $(OBJDIR)

install: $(TARGET)
	@echo "INSTALL $(BIN) -> $(DESTDIR)$(PREFIX)/bin/$(BIN)"
	@install -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	@echo "UNINSTALL $(DESTDIR)$(PREFIX)/bin/$(BIN)"
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

.PHONY: all clean install uninstall
