# sysmon(1)

## NAME

sysmon — resource monitor for Linux

## SYNOPSIS

`sysmon` [*OPTIONS*]

## DESCRIPTION

**sysmon** is a terminal-based resource monitor showing CPU, memory, network,
and process information with braille graphs and color themes.

## OPTIONS

`-h`, `--help`
: Print help and exit.

`-v`, `--version`
: Print version and exit.

`-t`, `--tty`
: Force TTY mode (256 colors, ASCII box chars).

`-l`, `--low-color`
: Force 256-color mode.

`-u` *MS*, `--update` *MS*
: Set update interval to *MS* milliseconds. Default: 2000.

`-f` *STR*, `--filter` *STR*
: Set initial process filter.

`--utf-force`
: Start even without detected UTF-8 locale.

## KEYBINDINGS

`q`, `Ctrl-C` — Quit  
`j`/`↓`, `k`/`↑` — Navigate process list  
`r` — Reverse sort  
`t` — Toggle tree view  
`f` — Filter input  
`Esc` — Clear filter  
`+`/`-` — Adjust update interval  
`F5` — Force redraw  

## FILES

`~/.config/sysmon/sysmon.conf` — user config  
`~/.config/sysmon/themes/` — user themes  
`/usr/local/share/sysmon/themes/` — system themes  

## SEE ALSO

top(1), htop(1)

## AUTHORS

0xtooda and sysmon contributors.
