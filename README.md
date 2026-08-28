# MGS4 Ultra120

Open-source ultrawide, FOV and controller-profile fixes for the Steam PC port
of *METAL GEAR SOLID 4*, with corrected 120 FPS support on Windows through
[cipherxof/MGSFPSUnlock](https://github.com/cipherxof/MGSFPSUnlock).

> **Public alpha.** `v0.3.1-alpha.5` targets the verified Steam executable.
> Other builds are blocked unless the user accepts the unsafe override. Back up
> saves and keep Steam's game files available for verification.

> **Experimental Windows preview.** `v0.3.1-alpha.6` adds optional internal
> supersampling and remains disabled by default. The validated alpha.5 release
> is still the recommended download when supersampling is not required.

## What the Windows release installs

| Component | Purpose |
|---|---|
| `winmm.dll` | Pinned Ultimate ASI Loader v9.7.4 |
| `scripts/MGS4Ultra120.asi` | Ultrawide/Hor+, FOV and controller-profile fixes |
| `mgs4_ultrawide.ini` | Shared MGS4Ultra120 settings |
| `scripts/MGSFPSUnlock.asi` | Corrected high-frame-rate implementation by cipherxof |
| `scripts/MGSFPSUnlock.ini` | Persistent FPS target; defaults to 120 |

Our old single-value FPS override is disabled and no longer writes the game's
FPS state. MGSFPSUnlock owns frame-rate timing and supplies separate fixes for
camera movement, character control, polygon demos, physics, ragdolls, cloth,
hair/bandana, wind and SPURS tasks. This avoids two plugins fighting over the
same setting.

MGSFPSUnlock is an independent source-published project. Because its repository
currently does not declare a redistribution license, its binary is **not
repackaged** in our downloads. Guided setup downloads version 0.1.0 directly
from its official GitHub release, requires the pinned archive and ASI SHA-256
values to match, and installs only its ASI and INI. Thank you to
[cipherxof](https://github.com/cipherxof) for the substantially improved FPS
unlock implementation.

## Rendering status

- 3440x1440 real-time 3D rendering is validated as correctly proportioned and
  Hor+ rather than a stretched 16:9 image.
- `FOVMultiplier=1.150` is the recommended 21:9 framing and closely matches the
  established RPCS3 ultrawide presentation. `1.000` remains available when the
  original vertical FOV is preferred. Width, height and FOV remain configurable.
- The unfinished UI/safe-area experiment is not part of the release binary.
  Menus, HUD and full-screen effects retain the game's original behavior.
- Pre-rendered Bink video is unchanged.
- Main now corrects the primary camera projection before rebuilding its
  view-projection matrices and CPU visibility planes. This synchronizes the
  wider rendered FOV with culling instead of exposing the former side pop-in.
- Native 3440x1440 testing at `1.150` has correct proportions and a working
  aiming crosshair. The external 5120x2160 crosshair report remains open until
  that exact resolution is reproduced; it is not yet claimed fixed.

## Windows downloads

Use the existing
[v0.3.1-alpha.5 release](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.1-alpha.5)
and choose one package:

1. **Setup EXE** — guided Steam detection, configuration, shortcuts and safe
   uninstall. It requires internet access once to fetch the verified
   MGSFPSUnlock 0.1.0 package.
2. **Portable ZIP** — extract it, then run `MGS4Ultra120-Setup.cmd`. It provides
   the same guided installation without registering a persistent application.
3. **Manual ZIP** — the most transparent copy-only route. It runs no script,
   but corrected 120 FPS requires two additional files from the official
   MGSFPSUnlock 0.1.0 ZIP; see the short instructions below.
4. **Complete ZIP** — portable setup, manual folder, source-facing notices and
   all documentation in one archive.

The EXE is unsigned, so SmartScreen or a browser may show an
unknown-publisher/reputation warning. The EXE is optional. Download only from
the official release and keep antivirus enabled. All project code and build
instructions are public and can be reviewed or built independently.

The separate
[v0.3.1-alpha.6 experimental supersampling preview](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.1-alpha.6)
is Windows-only, disabled by default and intended for users who explicitly want
to render above their physical output resolution. See
[experimental supersampling](docs/EXPERIMENTAL_SUPERSAMPLING.md).

### Brief manual installation

Close the game, extract the manual ZIP and copy its contents into the folder
that directly contains `mgs4.exe`:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
```

For corrected 120 FPS, download
[MGSFPSUnlock 0.1.0](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0),
open its ZIP and copy only these two files into the same `MGS4\scripts` folder:

```text
MGSFPSUnlock\scripts\MGSFPSUnlock.asi
MGSFPSUnlock\scripts\MGSFPSUnlock.ini
```

Do **not** copy its `d3d11.dll`, `winhttp.dll` or `wininet.dll`; our package
already provides the required ASI loader. Confirm that its INI contains
`TargetFrameRate = 120`, then launch normally through Steam.

See [Windows installation](docs/INSTALL_WINDOWS.md) and
[manual installation](docs/MANUAL_INI.md) for backup and removal details.

## Windows defaults and known display issue

The configurator selects the primary monitor's physical resolution, native-size
windowed presentation, FOV 1.150, corrected 120 FPS, controller-profile fix and
Unity-launcher bypass. Exclusive fullscreen is an advanced option.

On one mixed-refresh NVIDIA multi-monitor system, focus changes produced a red
sweep/flicker and Windows display WATCHDOG reports. Testing was clean after
G-SYNC/VRR was disabled, but that is not a universal diagnosis. The
configurator warns and never changes driver or Windows display settings. See
[Windows troubleshooting](docs/TROUBLESHOOTING_WINDOWS.md).

## Linux / Proton

Linux remains a separate Proton line. The alpha.5 Linux core package removes
the unconditional `gamemoderun` dependency and refuses to write a Gamescope
command when Gamescope is missing. It uses the game's normal FPS behavior; the
external corrected-120 route stays Windows-only until validated under Proton.
Nested Gamescope can still have KDE/Wayland fullscreen issues; verify
`gamescope --help`, use `Super+F` to toggle fullscreen, or return to the native
command. See [Linux installation](docs/INSTALL_LINUX.md).

## Technical outline

The port normally supplies a 16:9 perspective matrix even when the output is
ultrawide. MGS4Ultra120 recognizes the central camera projection and applies the
configured aspect/FOV before rebuilding the combined matrices and six CPU
visibility planes:

```text
adjusted_m11 = original_m11 / FOVMultiplier
new_m00 = sign(m00) * abs(adjusted_m11) / target_aspect
```

This keeps rendered framing and world culling synchronized. A renderer-level
hook remains only as a fallback for untouched 16:9 projection paths and rejects
already corrected target-aspect matrices to prevent double application.
The known central camera path is validated by its full perspective-matrix
structure rather than a fixed zoom ceiling. This keeps the configured FOV
active during tight in-engine close-ups instead of briefly reverting to the
uncorrected framing.

The patch never edits `mgs4.exe`. The optional direct-launch wrapper backs up
`Launcher/launcher.exe`, uses the game's official `mgs4_param` bootstrap and
restores the original only when ownership hashes still match.

Further reading:

- [Configuration](docs/CONFIGURATION.md)
- [Experimental supersampling](docs/EXPERIMENTAL_SUPERSAMPLING.md) (separate
  branch only; not included in alpha.5)
- [Controller profile fix](docs/CONTROLLER_FIX.md)
- [Direct-launch wrapper](docs/LAUNCHER_WRAPPER.md)
- [UI and video status](docs/UI_AND_VIDEO.md)
- [Technical notes](docs/TECHNICAL.md)
- [Development and reproducible builds](docs/DEVELOPMENT.md)

MGS4 Ultra120 is MIT-licensed. Third-party components retain their own terms;
see [third-party notices](THIRD_PARTY_NOTICES.md). No game files are included.
This project is not affiliated with or endorsed by KONAMI.
