# sysmon(1)

## NAME

sysmon — resource monitor for Linux

## SYNOPSIS

`sysmon` [*OPTIONS*]

## DESCRIPTION

**sysmon** is a terminal-based resource monitor showing CPU, memory, network,
and process information in a btop-style interface with braille graphs and
color themes.

## OPTIONS

`-h`, `--help`
: Print help and exit.

`-v`, `--version`
: Print version string and exit.

`-t`, `--tty`
: Force TTY mode. Uses only 256 colors and ASCII box characters.

`-l`, `--low-color`
: Force 256-color mode (no true color).

`-u` *MS*, `--update` *MS*
: Set update interval to *MS* milliseconds. Default: 2000.

`-f` *STR*, `--filter` *STR*
: Set initial process filter to *STR*.

`--utf-force`
: Start even if no UTF-8 locale is detected.

## KEYBINDINGS

`q`, `Ctrl-C`
: Quit.

`j` / `↓`, `k` / `↑`
: Navigate process list.

`r`
: Reverse sort order.

`t`
: Toggle tree view.

`f`
: Focus filter input.

`Esc`
: Clear filter.

`+` / `-`
: Increase / decrease update interval by 100ms.

`1`–`4`
: Toggle CPU / MEM / NET / PROC boxes.

`F5`
: Force full redraw.

## FILES

`~/.config/sysmon/sysmon.conf`
: User configuration file.

`~/.config/sysmon/themes/`
: User theme directory.

`/usr/local/share/sysmon/themes/`
: System-wide theme directory (when installed).

## ENVIRONMENT

`TERM`
: Used to detect color capabilities.

`LC_ALL`, `LANG`
: Must support UTF-8 for full rendering.

## SEE ALSO

top(1), htop(1), btop(1)

## AUTHORS

sysmon contributors. Inspired by btop++ by aristocratos.
