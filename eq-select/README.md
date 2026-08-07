# eq-select

Selects an EasyEffects output preset via dmenu.

## Dependencies

- dmenu
- easyeffects

## Installation

```sh
make
make install
```

By default, installs to `~/.local/bin`. Override with:

```sh
make install PREFIX=/usr/local
```

## Usage

```
eq-select [dmenu options...]
```

Any arguments are passed straight through to `menucmd`, after eq-select's own
`-p` prompt flag. Use this to set fonts, colors, line count, or to switch to
a compatible menu (e.g. `rofi -dmenu`) without rebuilding.

## Configuration

Edit `config.h` to customize:

```c
/* menu command; extra args passed on the command line are forwarded to it */
static const char menucmd[] = "dmenu";

/* Maximum number of presets */
#define MAX_PRESETS 64
```

After editing, run `make clean && make` to rebuild with new settings.

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *eqselect[] = { "eq-select", "-c", "-l", "5", "-nn", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_e,     spawn, {.v = eqselect } },
};
```

## License

See LICENSE file for license information.
