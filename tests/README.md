# Tests

This directory contains tests for sysmon.

## Running Tests

```bash
make test
# or
cd build && ctest
```

## Test Coverage

- `test_fmt_bytes` — unit tests for `Mem::fmt_bytes` formatting
- `test_theme` — verify theme color parsing
- `test_layout` — verify layout computation at various terminal sizes
- `test_graph` — verify braille graph data windowing

More tests welcome via PR.
