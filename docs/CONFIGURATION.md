# Configuration

Run the graphical configurator while the game is closed:

```powershell
# Windows
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1
```

```bash
# Linux
./scripts/linux/configure.sh gui
```

Each checkbox is independent. Selecting the controller fix does not enable
ultrawide or FPS changes; selecting 120 FPS does not enable the controller fix
or alter resolution. Profiles are only convenient presets and can be changed
afterwards.

## INI reference

`mgs4_ultrawide.ini` is stored beside `mgs4.exe`:

```ini
[Patch]
UltrawideEnabled=1
FPSOverrideEnabled=1
AllowUnsupportedExecutable=0

[Ultrawide]
Width=3440
Height=1440
FOVMultiplier=1.000
ConstrainUITo16x9=0

[FPS]
Limit=60
ToggleHotkey=F10
ToggleHotkeyModifiers=None

[Input]
ControllerProfileFixEnabled=1

[Launcher]
SkipUnityLauncher=0
Region=eu
SelfRegion=EU
Language=en
ControllerType=XBOX
```

### Ultrawide

- `UltrawideEnabled=0` leaves resolution, projection, FOV and UI hooks
  uninstalled.
- `Width` and `Height` define output resolution and target aspect ratio.
- `FOVMultiplier` accepts `0.500`-`2.000`. `1.000` preserves original vertical
  FOV; `1.150` is an optional wider view.
- `ConstrainUITo16x9=1` enables the D3D12 safe-area prototype. It is not a
  finished anchored-HUD solution and can also constrain full-screen effects.

### Frame rate

- `FPSOverrideEnabled=0` leaves the game's original frame limiter untouched.
- `Limit` accepts `30`, `60` or `120`.
- `ToggleHotkey` accepts `Off`, `F1`-`F24`, one letter or one digit when edited
  manually. The GUI exposes choices from F6 to F12.
- `ToggleHotkeyModifiers` may contain `Ctrl`, `Alt`, `Shift` and/or `Win`, for
  example `Ctrl+F10`. The hotkey exists only while the game is running and the
  FPS module is enabled.
- Each hotkey press writes one state transition between 60 and 120. It does
  not poll or rewrite the value every frame.

Use 60 FPS for normal play. The 120 option has reproduced a scripted-scene
stall with audio continuing and is intended for testing.

### Controller profile

- `ControllerProfileFixEnabled=1` preserves a connected native controller
  profile when the port incorrectly attempts to switch to keyboard profile 0.
- Set it to `0` for keyboard/mouse-only play or to test unmodified input.
- Disconnecting every controller clears the preserved profile. Reconnection is
  detected by the game normally; no virtual controller is created.

### Unsupported executables

The configurator compares `mgs4.exe` with the verified SHA-256. Unknown builds
are blocked by default. `AllowUnsupportedExecutable=1` permits an attempt after
an explicit warning. Known code-hook signatures are still verified, but data
offsets may have moved, so the override may crash. It does not add actual
support for that build.

### Launcher

`SkipUnityLauncher` records the configurator choice. The configurator also
installs or restores the actual wrapper; editing this key alone does not move
files. `Language` supports `en`, `sp`, `fr`, `it`, `ge` and `jp`. See
[Direct-launch wrapper](LAUNCHER_WRAPPER.md).

## Example combinations

| Desired result | Ultrawide | FPS override | Controller fix | Wrapper |
|---|---:|---:|---:|---:|
| Ultrawide only, original FPS/input | On | Off | Off | Either |
| 120 FPS only at 16:9 | Off | On/120 | Off | Either |
| Controller correction only | Off | Off | On | Either |
| Ultrawide plus controller correction | On | Off | On | Either |
| Everything | On | On | On | On |

Ready-made CLI presets include `stable`, `ui-safe`, `120`, `120-ui`,
`fps-only-120`, `ultrawide-only` and `controller-fix-only`. For example:

```bash
./scripts/linux/configure.sh controller-fix-only
./scripts/linux/configure.sh fps-only-120
```

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -Profile controller-fix-only
```

The log is `mgs4_ultrawide.log` beside the executable. Internal game-menu
resolution/FPS values may appear to save but are superseded only by whichever
patch modules are enabled.
