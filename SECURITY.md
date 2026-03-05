# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | ✅ Yes |

## Reporting a Vulnerability

If you discover a security vulnerability in sysmon, please **do not** open a public GitHub issue.

Instead, report it privately by emailing the maintainer or opening a [GitHub Security Advisory](https://github.com/0xtooda/sysmon/security/advisories/new).

Please include:
- A description of the vulnerability
- Steps to reproduce
- Potential impact
- Any suggested fix if you have one

You can expect an acknowledgement within 48 hours and a fix or mitigation within 14 days depending on severity.

## Scope

sysmon reads from Linux kernel interfaces (`/proc`, `/sys`) and runs entirely as the current user. It does not open network sockets, does not accept remote input, and does not require elevated privileges under normal use.

Known areas to report:
- Privilege escalation via crafted `/proc` entries
- Buffer overflows in process name or command parsing
- Unsafe handling of terminal escape sequences from process names
