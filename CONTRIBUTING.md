# Contributing to sysmon

Thank you for your interest in contributing!

## Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Build and test: `make && ./sysmon`
5. Commit with a clear message
6. Open a Pull Request

## Code Style

- C++23
- Tabs for indentation (tab width = 4)
- Keep each `.cpp` file focused on its module
- Avoid external dependencies — use only libc and Linux kernel interfaces

## Reporting Bugs

Open a GitHub Issue with:
- Your terminal emulator and OS version
- Terminal dimensions (`echo $COLUMNS $LINES`)
- Steps to reproduce
- Screenshot if visual

## Themes

New themes are welcome! Copy any btop-compatible `.theme` file into `themes/`  
and open a PR with a short description of the color palette.

## License

By contributing you agree your contributions are licensed under Apache-2.0.
