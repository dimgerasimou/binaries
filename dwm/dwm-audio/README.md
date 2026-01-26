# dwm-audio

A lightweight PulseAudio volume control utility for efficient audio management.

## Features

- Volume adjustment with customizable step size
- Absolute volume setting with configurable maximum limit
- Mute toggle and explicit mute control
- Separate sink (output) and source (input) device control
- Query current volume and mute status
- Optional post-execution hooks for status bar updates

## Dependencies

### Compile Time
- PulseAudio development libraries (`libpulse`)

### Run Time
- PulseAudio or PipeWire with `pipewire-pulse`

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
dwm-audio [-d device] [-s step] [-m max] command
```

### Devices
- `sink` - Output device (speakers/headphones) - default
- `source` - Input device (microphone)

### Commands
- `up` - Increase volume by step amount
- `down` - Decrease volume by step amount
- `set VALUE` - Set volume to absolute value (0.0-max)
- `mute` - Toggle mute state
- `mute on` - Enable mute
- `mute off` - Disable mute
- `get` - Print current volume (0.0-max)
- `status` - Print volume and mute status

### Options
- `-d DEVICE` - Device type (sink or source)
- `-s STEP` - Volume step for up/down commands (default: 0.05)
- `-m MAX` - Maximum volume limit (default: 1.5)
- `-h` - Show help information

### Examples

```sh
# Increase output volume by 5%
dwm-audio up

# Decrease output volume by 10%
dwm-audio -s 0.1 down

# Set output volume to 80%
dwm-audio set 0.8

# Toggle output mute
dwm-audio mute

# Increase microphone volume
dwm-audio -d source up

# Set volume with 100% maximum limit
dwm-audio -m 1.0 set 0.9

# Get current volume
dwm-audio get

# Get full status
dwm-audio status
```

## Configuration

Edit `config.h` to customize defaults:

```c
#define DEFAULT_STEP    0.05       /* 5% volume step */
#define DEFAULT_DEV     AUDIO_SINK /* default device */
#define DEFAULT_MAX_VOL 1.5        /* 150% maximum volume */

/* Post-execution hook (using execvp)
 * Example for dwmblocks status bar update:
 * static const char *hookargv[] = { "pkill", "-RTMIN+5", "dwmblocks", NULL };
 */
static const char *hookargv[] = { NULL };
```

After editing, run `make clean && make` to rebuild with new settings.

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *volup[]     = { "dwm-audio", "up", NULL };
static const char *voldown[]   = { "dwm-audio", "down", NULL };
static const char *voltoggle[] = { "dwm-audio", "mute", NULL };

static Key keys[] = {
    /* ... */
    { 0, XF86XK_AudioRaiseVolume,  spawn, {.v = volup } },
    { 0, XF86XK_AudioLowerVolume,  spawn, {.v = voldown } },
    { 0, XF86XK_AudioMute,         spawn, {.v = voltoggle } },
};
```

**Note:** You'll need to include `<X11/XF86keysym.h>` in your dwm config for the media key definitions.

## Volume Limits

By default, the maximum volume is set to 150% (1.5) to allow audio amplification beyond normal levels. This can be:

- Changed at compile time via `config.h`
- Overridden at runtime with `-m` flag
- Set to 100% (1.0) for stricter volume control

## License

See LICENSE file for license information.
