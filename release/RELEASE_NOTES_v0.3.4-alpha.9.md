# MGS4 Ultra120 v0.3.4-alpha.9 (pre-release)

This pre-release is intended as a fallback for future MGS4 updates. It has the
same features, defaults and visual behavior as alpha.8, but no longer depends
on fixed addresses inside `mgs4.exe`. **alpha.8 remains the recommended stable
release.** Use this pre-release if a game update has made alpha.8 stop working
and no validated release is available yet.

## What changed

- Every code, caller and data address used by both ASIs is located from
  masked byte signatures. Relative call and RIP-relative displacements are
  wildcards, so ordinary code and data movement does not break them.
- Addresses are resolved and cross-checked per feature group (resolution,
  camera/FOV, projection, reticle, controller fix and each Native Centered HUD
  group). Caller routes must call the resolved function, and data references
  must agree across independent native readers and writers.
- On the supported executable, every signature must match at its known
  address, exactly as before.
- On an unrecognized executable, `SignatureRelocation=1` (the default)
  installs each group only if all of its signatures are found exactly once and
  pass their cross-checks. A group that fails is skipped and named in the log.
  The log always states that the build has not been validated.

## Settings

New keys in `mgs4_ultrawide.ini` (both are optional; missing keys use the
defaults shown):

```ini
[Patch]
SignatureRelocation=1

[Diagnostics]
SignatureAudit=0
ForceSignatureRelocation=0
```

- `SignatureRelocation=0` restores the previous behavior: an unrecognized
  `mgs4.exe` receives no changes.
- `AllowUnsupportedExecutable=1` no longer applies fixed offsets; it now enables
  the same signature relocation.
- The two diagnostics are for troubleshooting and should stay at `0`.

## Validation

On native Windows at 3440x1440 with the 2026-09-11 executable:

- MSVC and MinGW builds and the complete test suite passed; the offline
  resolver test reproduced every known address both at its recorded location
  and by full signature search, and confirmed that corrupted or duplicated
  signatures are rejected;
- normal mode: every core hook and all six Native Centered HUD groups
  installed with no warnings;
- forced search mode, which follows the same path as an unknown future build:
  every signature was found exactly once, every group installed, all resolved
  addresses matched the known ones, and the live pause-map, Codec and Mission
  Briefing corrections applied.

Not yet tested: the other renderer (only one of DX11/DX12 was exercised),
Proton, and a real future game update. Signatures can follow moved code but
cannot prove that a changed routine still means the same thing, and some HUD
checks depend on in-game object layouts. If something looks wrong after an
update, set `SignatureRelocation=0` or disable Native Centered HUD, and report
the issue with `mgs4_ultrawide.log` and `mgs4_native_centered_hud.log`.

## Installing

Use the Windows setup EXE/portable ZIP or the Linux tarball as usual, or the
manual ZIP for a copy-only installation. Existing settings are preserved.
