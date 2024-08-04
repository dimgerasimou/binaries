# Binaries for dwmblocks

## Note

All except keybord have compile time dependency the libnotify library. Most icons need ttf-font-awesome to render and 
dwm with nerd font support and status2d patch (git)

## battery

### Usage

Reports the battery level, status and the power manager's status. (Need to be selected before compile time);

### Dependencies

- optimus-manager or envy (both are optional, disable in .c file)

## bluetooth

### Usage

Prints a menu with options for bluetooth, or optionaly open the tui settings. Returns the bluetooth state as an icon.

### Dependencies

- st
- bluteuith (AUR) (or bluetui)
- bluez
- bluez-utils

## date

### Usage

Returns current day, can send via notify a cute monthly calendar or optionaly launch google calendar in firefox.

### Dependencies

- firefox

## internet

### Usage

Returns state as an icon, can notify about connection settings or optionaly launch either tui settings or the dmenu-wifiprompt script.

### Dependencies

- st
- network-manager
- dmenu-wifiprompt (in this repo)

## kernel

### Usage

Returns current kernel version and the number of packages to be updated, notifies more analiticly about their source, macman or aur.

### Dependencies

- paru
- pacman-contrib (checkupdates)

## keyboard

### Usage

Returns the current keyboard layout.

### Dependencies

Compile time:

- libx11
- libxkbcommon

## memory

### Usage

Returns the memory that is currently used. Can execute htop.

### Dependencies

- st
- htop

## power

### Usage

Prints a power menu, with some extra functionality.

### Dependencies

- xmenu
- clipmenu
- dwmblocks
- slock
- optimus-manager

## time

### Usage

Returns the current time in 24h format.

### Dependencies

No dependecies.

## volume

### Usage

Returns current volume and state, notifies it along the default sources info. Can launch pulsemixer, or easyeffects.

### Dependencies

- wireplumber
- st
- audiocontrol (in this repo)
- easyeffects
