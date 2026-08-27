# MGS4 Ultra 120

An open-source ultrawide and experimental 120 FPS patch for the Steam PC port
of *METAL GEAR SOLID 4*. The patch fixes the 3D projection at arbitrary aspect
ratios instead of stretching a 16:9 image. It works before the renderer splits
into DirectX 11 and DirectX 12, so both backends use the same Hor+ correction.

> **Alpha software.** Back up your saves and read the compatibility notes.
> Version `v0.2.1-alpha.1` supports one known Steam executable build only.

## What works

- Native 3440x1440 output without pillarboxing.
- Correctly proportioned Hor+ 3D rendering, with additional horizontal view.
- The same engine-level projection fix in DirectX 11 and DirectX 12.
- Configurable width and height for other ultrawide resolutions.
- Optional proportion-preserving FOV multiplier (1.000 by default; 1.150 was
  visually validated as a wider comfort setting).
- An optional centered 16:9 UI safe area on DirectX 12 (experimental and off
  by default).
- Stable-by-default 60 FPS plus 30 and experimental 120 FPS values.
- Independent ultrawide/FOV and FPS modules: either can be disabled without
  applying its hooks, including a ready-made 120-FPS-only profile.
- Reversible Windows and Linux/Proton installers.
- Graphical Windows and Linux configurators for resolution, FOV, 30/60/120
  FPS, independent module switches, UI mode, and Linux fullscreen launch mode.

## Known limitations

- The selective 16:9 UI safe area is experimental. The identified shader is
  also used by some full-screen fades/effects, so enabling it can incorrectly
  narrow or overlap those effects. It is off by default. DirectX 11 receives
  the world/projection fix but not this UI experiment.
- Pre-rendered video remains 16:9 and is not expanded.
- **120 FPS is not considered gameplay-safe.** An intro/script stall with
  audio continuing was reproduced at 120 FPS. The release default is 60.
- A full playthrough and save compatibility are not yet certified. A completed
  load can wait indefinitely at `PULSE CUALQUIER BOTÓN`; that screen requires
  an explicit input and is not itself a loading stall.
- Only the Steam executable listed in
  [Compatibility](docs/COMPATIBILITY.md) is accepted. Unknown builds are not
  patched.

## Download and install

Download the package for your platform from
[Releases](https://github.com/drbermejor/mgs4Ultra120/releases).

- [Windows installation](docs/INSTALL_WINDOWS.md)
- [Linux / Proton installation](docs/INSTALL_LINUX.md)
- [Configuration and troubleshooting](docs/CONFIGURATION.md)

The patch does not modify `mgs4.exe` on disk. It installs a `winmm.dll` proxy
and an INI file beside the game executable. Both platform installers preserve
pre-existing files and provide a reversible uninstall path.

The DLL and INI normally remain in place after a Steam update, but the patch
does not guess offsets for a changed executable. It reports the new hash and
fails closed until that build is analyzed and added explicitly.

## How the fix works

The port normally supplies 16:9 perspective matrices even when its final
canvas is ultrawide. The DLL hooks the common engine projection setter and,
for matrices positively identified as 16:9 perspective projections, replaces
only the horizontal scale:

```text
adjusted_m11 = original_m11 / FOVMultiplier
new_m00 = sign(m00) * abs(adjusted_m11) / target_aspect
```

At 1.000, vertical FOV is preserved and horizontal FOV grows. Other multiplier
values change both axes together, so geometry keeps its proper proportions.
Resolution setters/getters apply the output size when state
changes; there is no 8 ms memory-rewrite loop. On D3D12, one positively
identified UI shader receives a centered 16:9 viewport for its draw calls,
after which the original viewport is restored. This path is opt-in until UI
and full-screen effect draws can be distinguished reliably. See
[Technical notes](docs/TECHNICAL.md) for the safeguards and known offsets.

## Building and contributing

Build instructions, the pinned MinHook dependency, and the release packaging
process are documented in [Development](docs/DEVELOPMENT.md). Contributions
are welcome; please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

This project is licensed under the [MIT License](LICENSE). MinHook retains its
own BSD license; see [third-party notices](THIRD_PARTY_NOTICES.md).

MGS4 Ultra 120 is an unofficial community project. It contains no game files
and is not affiliated with or endorsed by KONAMI.
