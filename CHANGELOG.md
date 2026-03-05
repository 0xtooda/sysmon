# Changelog

## [0.1.0] — 2025

### Added
- Initial release
- CPU box with braille history graph and per-core dot-bar meters
- GPU line (NVIDIA + AMD) showing usage %, VRAM, temperature
- Memory box with RAM stats and disk usage bars
- Network box with download/upload braille graphs and stats
- Process table: Pid, Program, Command, Threads, User, MemB, Cpu%
- Process sorting, filtering, reverse sort, tree view
- Menu bar with clock and update interval
- Themes: Gruvbox (default), Nord, Dracula, Tokyo Night, One Dark
- Responsive layout (min 80×24)
- TTY/256-color fallback mode
- Config file at `~/.config/sysmon/sysmon.conf`
- GPU detection: NVIDIA via `nvidia-smi`, AMD via `/sys/class/drm`
- `make install` / `make uninstall`
- CMake build support
