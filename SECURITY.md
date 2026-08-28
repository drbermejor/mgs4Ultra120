# Security policy

This patch loads native code into a single-player game process and writes to
validated in-memory addresses. Download releases from this repository and use
the published SHA-256 files when verifying binaries.

The project is fully open source. The installer, patch, setup scripts, build
instructions and pinned third-party metadata can all be inspected before use,
and the project can be compiled locally. The distributed EXE is not digitally
signed, so an unknown-publisher or reputation notice is possible. Running the
EXE is optional: the portable ZIP includes the readable CMD/PowerShell setup and
also supports a copy-only manual installation documented in
`docs/MANUAL_INI.md`.

Please report a vulnerability privately through GitHub Security Advisories if
possible. Do not include proprietary game binaries or sensitive user data in a
report.
