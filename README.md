# sysmon

A lightweight, btop-style TUI system monitor written in C++23.

```
 ███████╗██╗   ██╗███████╗███╗   ███╗ ██████╗ ███╗   ██╗
 ██╔════╝╚██╗ ██╔╝██╔════╝████╗ ████║██╔═══██╗████╗  ██║
 ███████╗ ╚████╔╝ ███████╗██╔████╔██║██║   ██║██╔██╗ ██║
 ╚════██║  ╚██╔╝  ╚════██║██║╚██╔╝██║██║   ██║██║╚██╗██║
 ███████║   ██║   ███████║██║ ╚═╝ ██║╚██████╔╝██║ ╚████║
 ╚══════╝   ╚═╝   ╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
```

**Version:** 0.1.0  
**License:** Apache-2.0  
**Platform:** Linux (x86_64, aarch64)

---

## Features

- Real-time **CPU** usage with per-core meters and braille history graph
- **Memory** and swap usage with disk stats
- **Network** upload/download graphs per interface
- **Process** table sorted by CPU%, with filter and tree view
- **GPU** support (NVIDIA via nvidia-smi, AMD via amdgpu sysfs)
- Multiple themes (Gruvbox, Nord, Dracula + all btop-compatible themes)
- Fully responsive — adapts to any terminal size ≥ 80×24
- Written in C++23, zero runtime dependencies beyond libc

---

## Screenshots

> _Run `./sysmon` in a terminal that supports UTF-8 and true color._

---

## Requirements

| Dependency | Version |
|------------|---------|
| GCC or Clang | C++23 support (GCC ≥ 13, Clang ≥ 16) |
| Linux kernel | ≥ 3.0 (uses `/proc` and `/sys`) |
| Terminal | UTF-8, 256-color or true-color recommended |

---

## Build

```bash
# Clone / extract
cd sysmon

# Build with make
make

# Or with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Build output: `./sysmon`

---

## Install

```bash
sudo make install        # installs to /usr/local/bin/sysmon
sudo make uninstall      # removes it
```

---

## Usage

```
sysmon [options]

Options:
  -h, --help            Show this help message
  -v, --version         Print version and exit
  -t, --tty             Force TTY mode (256 colors, no unicode box chars)
  -l, --low-color       Force 256-color mode
  -u, --update <ms>     Set update interval in milliseconds (default: 2000)
  -f, --filter <str>    Set initial process filter string
      --utf-force       Force start even without detected UTF-8 locale
```

### Keybindings

| Key | Action |
|-----|--------|
| `q` / `Ctrl-C` | Quit |
| `j` / `↓` | Select next process |
| `k` / `↑` | Select previous process |
| `r` | Reverse sort order |
| `t` | Toggle tree view |
| `f` | Focus filter input |
| `Esc` | Clear filter |
| `+` / `-` | Increase / decrease update interval |
| `1`–`4` | Toggle CPU / MEM / NET / PROC boxes |
| `F5` | Force redraw |

---

## Themes

Themes live in `./themes/` (or `/usr/local/share/sysmon/themes/` when installed).  
Pass a theme name via config, or copy any btop-compatible `.theme` file into the themes directory.

Included themes:
- `Gruvbox` (default)
- `Nord`
- `Dracula`

Any `.theme` file from [btop's themes](https://github.com/aristocratos/btop/tree/main/themes) is compatible.

---

## Configuration

Config file is created automatically at `~/.config/sysmon/sysmon.conf` on first run.

Key options:

```ini
# Update interval in milliseconds
update_ms = 2000

# Color theme name (filename without .theme extension)
color_theme = Gruvbox

# Show temperatures (requires coretemp/k10temp kernel module)
check_temp = True

# Process sort field: "cpu lazy", "mem", "pid", "name"
proc_sorting = cpu lazy

# Reverse process sort order
proc_reversed = False

# Show processes as tree
proc_tree = False
```

---

## GPU Support

**NVIDIA:** Requires `nvidia-smi` in `$PATH`. No extra packages needed.

**AMD:** Reads from `/sys/class/drm/cardX/device/` sysfs. Requires `amdgpu` kernel module (loaded by default on modern kernels).

If no GPU is detected the GPU line is simply hidden.

---

## Project Structure

```
sysmon/
├── src/
│   ├── sysmon.cpp      Main loop, signal handlers, CLI args
│   ├── draw.cpp        TUI rendering (all boxes, graphs, meters)
│   ├── cpu.cpp         /proc/stat, frequency, temperature, uptime
│   ├── mem.cpp         /proc/meminfo, disk stats
│   ├── net.cpp         /proc/net/dev, interface detection
│   ├── proc.cpp        /proc/<pid>/* process collection
│   ├── config.cpp      Config file read/write
│   ├── theme.cpp       Color theme loading and gradients
│   └── input.cpp       Keyboard input handling
├── include/            Header files for all modules
├── themes/             Color theme files
├── Img/                Logo and screenshots
├── Makefile
└── CMakeLists.txt
```

---

## License

Copyright 2025 sysmon contributors.  
Licensed under the [Apache License 2.0](LICENSE).

---

## Acknowledgements

Inspired by [btop++](https://github.com/aristocratos/btop) by aristocratos.  
Braille graph rendering technique adapted from btop's open-source implementation.
