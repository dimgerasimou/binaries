# binaries - build/install wrapper for all projects
# See LICENSE file for copyright and license details.

MAKEFLAGS += --no-print-directory

PREFIX  ?= $(HOME)/.local

TARGETS := audio-ctl eq-select selshot sink-select wifi-prompt xkbnext trackpad-toggle kbd-backlight
SCRIPTS := wallpaper-select

all: $(TARGETS)

$(TARGETS):
	@$(MAKE) -C $@ PREFIX=$(PREFIX) all

clean:
	@for t in $(TARGETS); do $(MAKE) -C $$t clean; done

install: all
	@for t in $(TARGETS); do \
		$(MAKE) -C $$t DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) install; \
	done
	@echo "INSTALL $(SCRIPTS) -> $(DESTDIR)$(PREFIX)/bin"
	@install -d $(DESTDIR)$(PREFIX)/bin
	@for s in $(SCRIPTS); do \
		install -m 755 $$s/$$s $(DESTDIR)$(PREFIX)/bin/$$s; \
	done

uninstall:
	@for t in $(TARGETS); do \
		$(MAKE) -C $$t DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) uninstall; \
	done
	@for s in $(SCRIPTS); do \
		rm -f $(DESTDIR)$(PREFIX)/bin/$$s; \
	done

.PHONY: all clean install uninstall $(TARGETS)
