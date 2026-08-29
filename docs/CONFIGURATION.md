# Configuration

Run a configurator only while the game is closed. On Linux use:

```bash
./MGS4Ultra120-Linux-Configure.sh
```

On Windows use:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -GameDir "D:\SteamLibrary\steamapps\common\METAL GEAR SOLID 4\MGS4"
```

Easy Setup supplies the selected game directory automatically. The GUI saves
MGS4Ultra120 settings to `MGS4\mgs4_ultrawide.ini` and the frame-rate target to
`MGS4\scripts\MGSFPSUnlock.ini`. The independent centered-HUD switch is stored
in `MGS4\mgs4_centered_hud_16x9.ini`.

## MGS4Ultra120 INI

```ini
[Patch]
UltrawideEnabled=1
FPSOverrideEnabled=0
AllowUnsupportedExecutable=0

[Ultrawide]
Width=3440
Height=1440
FOVMultiplier=1.200
FOVModelVersion=2
NativeCameraFOV=1
ExperimentalCinematicFOV=0
CinematicFOVMultiplier=inherit

[FPS]
Limit=60

[Input]
ControllerProfileFixEnabled=1

[Launcher]
SkipUnityLauncher=1
Region=eu
SelfRegion=EU
Language=en
ControllerType=XBOX
DisplayMode=Windowed
UsePrimaryPhysicalResolution=1
```

- `UltrawideEnabled=1` enables native resolution and Hor+ projection changes.
- `Width`/`Height` accept physical output dimensions. The GUI supports
  640–16384 by 480–16384 and can read the primary monitor's physical mode.
- `FOVMultiplier` accepts any finite value of `0.500` or greater. `1.200` is
  the tested 21:9 recommendation with the corrected single-owner camera route;
  `1.000`
  preserves the game's original vertical FOV but frames Snake more tightly in
  the tested view. Native camera ownership keeps projection and visibility data
  synchronized. Values above `1.200` are untested and may reveal geometry or
  animation transitions early, produce unusual framing or cause instability;
  they are accepted under the user's responsibility. Both `.`
  and `,` decimal separators are accepted.
- `FOVModelVersion=2` records the single-owner model. Managed Windows updates
  migrate the old repeated-route default `1.050` to `1.200` once; later manual
  choices are preserved.
- `NativeCameraFOV=1` enables the tested single-owner route but remains
  experimental in this alpha. Set it to `0` first if instability or a
  scene-specific regression appears; Hor+ remains enabled with the game's
  original vertical FOV. Both configurators expose this switch.
- `ExperimentalCinematicFOV=1` opts into the separate in-engine cinematic
  camera preview. It requires `NativeCameraFOV=1`, is `0` by default and does
  not affect pre-rendered video.
- `CinematicFOVMultiplier=inherit` uses the gameplay `FOVMultiplier`. Advanced
  testers may enter a separate finite value of `0.500` or greater. Expanded
  cinematic framing can reveal characters, objects or animation transitions
  before the authored shot intended them to enter the frame.
- `mgs4_centered_hud_16x9.ini` uses `Enabled=1` to place conservatively
  identified HUD draws in a centered 16:9 safe area. It is experimental,
  disabled by default and DX12-only. `Enabled=0` exits before installing D3D12
  hooks.
- `ControllerProfileFixEnabled=1` preserves the native connected-controller
  family when the port incorrectly attempts to switch to keyboard profile 0.
- `AllowUnsupportedExecutable=1` attempts known offsets on an unverified build
  and may crash. It is off by default.
- `DisplayMode=Windowed` is recommended. `Fullscreen` is an advanced option.
- `UsePrimaryPhysicalResolution=1` refreshes dimensions from Win32 rather than
  DPI-scaled desktop bounds when the GUI saves.
- `Language` accepts `en`, `sp`, `fr`, `it`, `gr`, `jp` and `pt`. Current
  Windows setup synchronizes this selection with both launcher paths. Legacy
  `ge` is migrated to the game's correct German token, `gr`. **Recommended
  settings** changes rendering/input defaults but deliberately preserves the
  selected language.

`FPSOverrideEnabled` and `[FPS] Limit` remain only so older files and tools can
be migrated safely. The current Windows ASI ignores them and setup always forces
the old override off.

## Corrected FPS INI

MGSFPSUnlock owns frame-rate timing:

```ini
[Settings]
TargetFrameRate = 120
```

Both GUIs expose 120, 60 and 30 when the optional component is installed.
Guided setup installs and verifies MGSFPSUnlock 0.1.0 before writing this file;
Linux also applies and verifies the Wine compatibility byte locally. Do not enable an older
`FPSOverrideEnabled=1` at the same time.

## Command-line profiles

```powershell
# Recommended ultrawide/controller setup plus corrected 120 FPS
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -GameDir "D:\...\MGS4" -Profile stable

# Corrected 120 FPS only; no ultrawide or controller-profile hook
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -GameDir "D:\...\MGS4" -Profile fps-only-120

# 1920x1080 output, 3840x2160 internal render, no ultrawide/FOV changes
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -GameDir "D:\...\MGS4" -Profile 16x9-supersampling
```

Other profiles are `ultrawide-only` and `controller-fix-only`. MGSFPSUnlock
remains installed at 120 because it is the independent frame-rate component;
choose 60 in the GUI if desired.

Every command-line profile disables cinematic FOV and centered HUD. All except
`16x9-supersampling` also disable supersampling. This makes `-Profile stable` a
direct software fallback without requiring reinstallation.

The project log is `mgs4_ultrawide.log`. MGSFPSUnlock also writes its own log in
the game directory; include both when reporting a high-FPS problem.
