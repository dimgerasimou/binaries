# kbd-backlight

`kbd-backlight` is a simple utility to control single-zone RGB keyboard
backlight via [OpenRGB](https://openrgb.org/)).

## Build & install

Edit the `Makefile` to match your configuration.
To install simply run:

```bash
make install
```

Requires `openrgb` on runtime.

## Usage

```bash
kbd-backlight off
kbd-backlight medium
kbd-backlight full
kbd-backlight toggle                    # off -> medium -> full -> off
kbd-backlight status                    # level / color / brightness

kbd-backlight full --color 00ff00       # set + save color, apply now
kbd-backlight custom --brightness 33    # arbitrary brightness, saved,
```

`--color` works with all commands and persists all future changes.
`--brightness` works only with `custom`, `medium`/`full` always
use the fixed levels from `config.h`.

## Config

Edit `config.h` (copied from `config.def.h` on first build) for any changes
and then recompile.

