# src/linux

Linux-specific platform implementations.

This directory contains Linux kernel interface code used by sysmon's core modules.

| File | Description |
|------|-------------|
| `cpu_linux.cpp` | `/proc/stat` parsing, frequency scaling, temperature via hwmon |
| `mem_linux.cpp` | `/proc/meminfo`, `/proc/mounts`, statvfs disk stats |
| `net_linux.cpp` | `/proc/net/dev` interface enumeration and speed calculation |
| `proc_linux.cpp` | `/proc/<pid>/stat`, `/proc/<pid>/status`, `/proc/<pid>/cmdline` |

> Platform code is compiled directly into the main modules for v0.1.0.
> This directory is reserved for the upcoming platform abstraction layer.
