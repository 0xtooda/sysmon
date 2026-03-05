# Contributing to sysmon

Thanks for your interest!

## Getting Started

1. Fork the repo
2. Create a branch: `git checkout -b feature/my-feature`
3. Make changes, build and test: `make && ./sysmon`
4. Commit with a clear message
5. Open a Pull Request

## Code Style

- C++23
- Tabs for indentation (width = 4)
- Keep each `.cpp` file focused on its module
- No external dependencies — only libc and Linux kernel interfaces

## Reporting Bugs

Open a GitHub Issue with:
- OS and kernel version
- Terminal emulator
- Terminal dimensions (`echo $COLUMNS $LINES`)
- Steps to reproduce + screenshot

## Themes

New themes are welcome — add a `.theme` file to `themes/` and open a PR.

## License

Contributions are licensed under Apache-2.0.
