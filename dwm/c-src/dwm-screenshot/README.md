# dwm-screenshot

A lightweight screenshot utility that wraps `maim` with customizable selection borders and automatic file organization.

## Features

- Interactive region selection with customizable border color and size
- Automatic directory creation with configurable path expansion (`~`, `$VAR`, `${VAR}`)
- Timestamped filenames (e.g., `Screenshot-2026-01-25-143022.png`)
- Desktop notifications via libnotify
- Full-screen capture mode

## Dependencies

- [maim](https://github.com/naelstrof/maim) - Screenshot utility
- libnotify - Desktop notifications
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

## Usage

```
dwm-screenshot [-b bordersize] [-c #RRGGBB] [-o 0xAA] [-fh]
```

### Options

- `-b <size>` - Selection border size in pixels (default: 3)
- `-c #RRGGBB` - Border color in hex RGB format (default: #EEEEEE)
- `-o 0xAA` - Border opacity in hex (0x00-0xFF, default: 0xFF)
- `-f` - Full-screen capture (skips selection)
- `-h, --help` - Show usage information

### Examples

```sh
# Interactive selection with defaults
dwm-screenshot

# Full-screen capture
dwm-screenshot -f

# Custom border: 5px, red, 50% opacity
dwm-screenshot -b 5 -c #FF0000 -o 0x80

# Thin blue border
dwm-screenshot -b 1 -c #0088FF
```

## Configuration

Edit the source file to customize:

```c
static const char scrdirpath[]   = "~/Pictures/Screenshots/";
static const char imgextension[] = "png";

#define DEFAULT_BORDERSIZE 3
#define DEFAULT_COLOR      0xFFEEEEEE  /* ARGB */
#define DEFAULT_FSCR       0
```

The screenshot directory supports:
- Tilde expansion: `~/Pictures/Screenshots/`
- Environment variables: `$HOME/Screenshots/` or `${XDG_PICTURES_DIR}/Screenshots/`

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *screenshot[] = { "dwm-screenshot", NULL };
static const char *screenshotfull[] = { "dwm-screenshot", "-f", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_Print, spawn, {.v = screenshot } },
    { MODKEY|ShiftMask,     XK_Print, spawn, {.v = screenshotfull } },
};
```

## License

See LICENSE file for license information.
