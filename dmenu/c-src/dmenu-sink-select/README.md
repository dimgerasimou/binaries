# dmenu-sink-select

A lightweight PulseAudio sink selector using dmenu for quick audio output device switching.

## Features

- Interactive sink selection via dmenu
- Filters ignored devices from configurable ignore list
- Optional post-selection hooks for status bar updates
- Minimal dependencies and resource usage

## Dependencies

### Compile Time
- PulseAudio development libraries (`libpulse`)

### Run Time
- PulseAudio or PipeWire with `pipewire-pulse`
- [dmenu](https://tools.suckless.org/dmenu/) or compatible menu (e.g., rofi with `-dmenu` flag)

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
dmenu-sink-select [dmenu options...]
```

All arguments are passed directly to dmenu, allowing full customization of the menu appearance.

### Examples

```sh
# Basic usage with default dmenu appearance
dmenu-sink-select

# Custom dmenu font and colors
dmenu-sink-select -fn 'monospace:size=12' -nb '#1e1e2e' -nf '#cdd6f4'

# Use with rofi instead of dmenu
dmenu-sink-select -dmenu rofi -dmenu

# Case-insensitive matching
dmenu-sink-select -i
```

## Configuration

Edit `config.h` to customize:

```c
static const char menucmd[] = "dmenu";

/* Post-selection hook (using execvp)
 * Example for dwmblocks status bar update:
 * static const char *hookargv[] = { "pkill", "-RTMIN+5", "dwmblocks", NULL };
 * 
 * Example for dwmblocksctl:
 * static const char *hookargv[] = { "dwmblocksctl", "-s", "volume", NULL };
 */
static const char *hookargv[] = { NULL };
```

After editing, run `make clean && make` to rebuild with new settings.

### Ignoring Sinks

To hide specific audio devices from the menu, create an ignore list at:

```
$XDG_CONFIG_HOME/dmenu/dmenu-sink-select/ignore-sinks
```

Or if `XDG_CONFIG_HOME` is not set:

```
~/.config/dmenu/dmenu-sink-select/ignore-sinks
```

Add one sink description per line (exact match required):

```
HDMI / DisplayPort
Dummy Output
```

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *sinkselector[] = { "dmenu-sink-select", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_F9,    spawn, {.v = sinkselector } },
};
```

## How It Works

1. Queries PulseAudio for all available output devices (sinks)
2. Filters out devices listed in the ignore file
3. Presents remaining devices in dmenu
4. Sets the selected device as the default output
5. Optionally executes a hook command (e.g., to update status bars)

## License

See LICENSE file for license information.