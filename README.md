<div align="center">

```
 ███████╗██╗   ██╗███████╗███╗   ███╗ ██████╗ ███╗   ██╗
 ██╔════╝╚██╗ ██╔╝██╔════╝████╗ ████║██╔═══██╗████╗  ██║
 ███████╗ ╚████╔╝ ███████╗██╔████╔██║██║   ██║██╔██╗ ██║
 ╚════██║  ╚██╔╝  ╚════██║██║╚██╔╝██║██║   ██║██║╚██╗██║
 ███████║   ██║   ███████║██║ ╚═╝ ██║╚██████╔╝██║ ╚████║
 ╚══════╝   ╚═╝   ╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
```

**A fast, feature-rich TUI system monitor written in C++23**

[![Build](https://github.com/0xtooda/sysmon/actions/workflows/build.yml/badge.svg)](https://github.com/0xtooda/sysmon/actions/workflows/build.yml)
![License](https://img.shields.io/badge/license-Apache--2.0-blue)
![Version](https://img.shields.io/badge/version-0.1.0-green)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![Language](https://img.shields.io/badge/language-C%2B%2B23-orange)

</div>

---

## Features

- **CPU** — full-width braille history graph, per-core meters, temperature, frequency, load average
- **GPU** — usage %, VRAM used/total, temperature (NVIDIA + AMD)
- **Memory** — Total / Used / Available / Cached / Free / Swap with disk usage bars
- **Network** — download and upload braille graphs with live speed and totals
- **Processes** — Pid, Program, Command, Threads, User, MemB, Cpu% — sortable, filterable, tree view
- **Themes** — Gruvbox (default), Nord, Dracula, Tokyo Night, One Dark
- **Responsive** — adapts to any terminal size ≥ 80×24
- **Zero dependencies** — only libc and Linux kernel interfaces (`/proc`, `/sys`)

---

## Requirements

| | |
|---|---|
| Compiler | GCC ≥ 13 or Clang ≥ 16 (C++23) |
| OS | Linux (kernel ≥ 3.0) |
| Terminal | UTF-8, 256-color or true-color |

---

## Build & Run

```bash
git clone https://github.com/0xtooda/sysmon.git
cd sysmon
make
./sysmon
```

Or with CMake:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/sysmon
```

Install system-wide:
```bash
sudo make install
sysmon
```

---

## Keybindings

| Key | Action |
|-----|--------|
| `q` / `Ctrl-C` | Quit |
| `j` / `↓` | Next process |
| `k` / `↑` | Previous process |
| `r` | Reverse sort |
| `t` | Toggle tree view |
| `f` | Filter processes |
| `Esc` | Clear filter |
| `+` / `-` | Adjust update interval |
| `F5` | Force redraw |

---

## Themes

Drop any `.theme` file into `~/.config/sysmon/themes/` and set `color_theme` in `~/.config/sysmon/sysmon.conf`.

---

## Project Structure

```
sysmon/
├── src/
│   ├── linux/         Linux platform code
│   └── *.cpp          Core modules
├── include/           Header files
├── themes/            Color themes
├── Makefile
└── CMakeLists.txt
```

---

## License
<img width="1897" height="1032" alt="image copy 5" src="https://github.com/user-attachments/assets/0341e1cd-ac0c-4d68-bdd5-2ed9e7d383bf" />

Apache-2.0 © 2025 0xtooda
