# Security policy

This patch loads local native code into a single-player game process and writes
to validated in-memory addresses. Alpha.3 uses a combined proxy DLL; the
alpha.4 preview uses pinned Ultimate ASI Loader plus a project ASI. Only
download releases from this repository and verify every published SHA-256.

The installer and project binaries are unsigned, and the upstream loader's
certificate is self-signed rather than rooted in the Windows trusted publisher
store. Chrome, SmartScreen or antivirus can show reputation warnings. Do not
disable protection globally or assume every warning is false; use the portable
ZIP/CMD route when you do not trust the installer EXE.

Please report a vulnerability privately through GitHub Security Advisories if
possible. Do not include proprietary game binaries or sensitive user data in a
report.
