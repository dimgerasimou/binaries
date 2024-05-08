# Wifi prompt

## Usage

A fast and simple wifi prompt made to use dmenu. It works with open or wpa-psk authenticated networks. It checks connectivity and if something is wrong, then it deletes the connection. Signals dwmblocks to update using my `dwmblocksctl`. Notifies of the results using libnotify.

## Dependencies

- libnotify (buildtime)
- libnm (buildtime)
- dwmblocksctl (runtime)
- dmenu (runtime)

## Arguments

Takes no input argument. Calls NetworkManager through libnm. Gets input through dmenu. Outputs error logs in `$HOME/window_manager.log`.