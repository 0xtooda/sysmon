# Changelog

All notable changes to sysmon will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [0.1.0] — 2025

### Added
- Initial release
- CPU box with full-width braille history graph and per-core dot-bar meters
- GPU line showing usage %, VRAM used/total, and temperature (NVIDIA + AMD)
- Memory box with RAM stats (Total/Used/Available/Cached/Free/Swap)
- Disk usage bars per mounted filesystem
- Network box with separate download/upload braille graphs and stats panel
- Process table with columns: Pid, Program, Command, Threads, User, MemB, Cpu%
- Process sorting by CPU% (default), memory, pid, name
- Process filter, reverse sort, tree view toggle
- Menu bar with clock and update interval display
- Three built-in themes: Gruvbox (default), Nord, Dracula
- Compatible with all btop `.theme` files
- Responsive layout — adapts to terminal size ≥ 80×24
- TTY mode (256-color fallback)
- Config file at `~/.config/sysmon/sysmon.conf`
- Signal handlers: SIGWINCH (resize), SIGINT (quit), SIGTSTP/SIGCONT (suspend/resume)
- GPU detection: NVIDIA via `nvidia-smi`, AMD via `/sys/class/drm` sysfs
- `make install` / `make uninstall` targets
- CMake build system as alternative to Make
