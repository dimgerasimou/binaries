# Binaries for dwm

## audiocontrol

### Usage

Executing this with a given set of arguments raises or decreses the volume by 5%
of the default audio source or sink, then signals dwmblocks to update the corresponding block.

### Examples

``` bash
	./audiocontrol source increase

```
Increacess the volume by 5% to the default source.

``` bash
	./audiocontrol sink mute

```
Toggles the mute property of the default sink.

### Dependencies

- Wireplumber
- dwmblocksctl (Binary can be found in my own build of dwmblocks.)

## mediacontrol

### Usage

This execuatble gets the first mpris client, or one selected from `$HOME/.local/state/dwm/mprisclient` for playback control
with one of the following arguments:
- toggle - toggles play - pause
- stop - stops playback
- next - moves to next track
- prev - moves to previous track

### Dependencies

Has no dependencies.

## takescreenshot

### Usage

Running this executable, takes a screenshot with scrot and saves it at `$HOME/Pictures/Screenshots` with the iso date as filename.

### Dependenices
Runtime:
- scrot

Compile time:
- libnotify

## dwm.desktop

### Usage 

This is simply a desktop entry for dwm, to be copied in the `xsessions` folder for dwm support with login managers.

## layoutmenu

### Usage

This is a menu with all dwm window layouts I have configured. Uses xmenu to print all of them and returns the one clicked.

### Dependencies

- xmenu