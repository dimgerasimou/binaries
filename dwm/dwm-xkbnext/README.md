# dwm-xkbnext

A lightweight utility that provides the ability to the user to change XKB layouts.

## Dependencies

- X11

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

Edit `config.h` file to customize:

```c

static const char *hookargv[] = { NULL };

```

If set, runs a hook that can be used to notify other applications about the layout change.

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *xkbnext[] = { "dwm-xkbnext", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_space, spawn, {.v = xkbnext } },
};
```

## License

See LICENSE file for license information.
