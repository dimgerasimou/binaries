# wifi-prompt

## Usage

```
wifi-prompt [dmenu options...]
```

A fast and simple wifi prompt made to use dmenu. It works with open or wpa-psk authenticated networks. It checks connectivity and if something is wrong, then it deletes the connection. Signals dwmblocks to update using my `dwmblocksctl`. Notifies of the results using libnotify.

Any arguments are passed straight through to `menucmd`, after wifi-prompt's own `-p` prompt flag (and `-P` on the password prompt). Use this to set fonts, colors, line count, or to switch to a compatible menu without rebuilding.

## Dependencies

- libnotify (buildtime)
- libnm (buildtime)
- dwmblocksctl (runtime)
- dmenu (runtime)

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
/* menu command; extra args passed on the command line are forwarded to it */
static const char menucmd[] = "dmenu";
```

After editing, run `make clean && make` to rebuild with new settings.

## Arguments

Calls NetworkManager through libnm. Gets input through dmenu. Outputs error logs in `$HOME/window_manager.log`.
