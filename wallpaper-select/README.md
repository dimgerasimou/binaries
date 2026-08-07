# wallpaper-select

Prompts for a wallpaper via dmenu and applies it.

## Usage

Executing the script prompts you to select a wallpaper from
`$HOME/.local/share/wallpapers`, converts it to jpg if needed, saves it as
`$HOME/.local/state/dwm/wallpaper.jpg`, and sets it with `feh --bg-fill`.

## Dependencies

- dmenu
- mogrify (ImageMagick)
- feh

## Installation

This is a plain shell script, no build step required:

```sh
install -Dm755 wallpaper-select "$HOME/.local/bin/wallpaper-select"
```

## Keybinding Example (dwm)

Add to your `config.h`:

```c
static const char *wallpaperselect[] = { "wallpaper-select", NULL };

static Key keys[] = {
    /* ... */
    { MODKEY,               XK_w,     spawn, {.v = wallpaperselect } },
};
```

## License

See LICENSE file for license information.
