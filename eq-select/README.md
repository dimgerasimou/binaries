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

## Configuration

Edit `config.h` to customize:

```c
/* dmenu configuration */
static const char *dmenu[] = { "dmenu", "-c", "-l", "5", "-p", "Select equalizer preset:", "-nn", NULL };

/* Maximum number of presets */
#define MAX_PRESETS 64
```

After editing, run `make clean && make` to rebuild with new settings.

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *eqselect[] = { "eq-select", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_e,     spawn, {.v = eqselect } },
};
```

## License

See LICENSE file for license information.
