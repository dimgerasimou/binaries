# eq-select

Selects an EasyEffects output preset via a menu. (default dmenu)

## Dependencies

- `dmenu` (or another `dmenu`-compatible menu, see Usage)
- `easyeffects`, with an EasyEffects instance already running in the background.

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
`-p` prompt flag. Use this to set fonts, colors, line count without rebuilding.

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
