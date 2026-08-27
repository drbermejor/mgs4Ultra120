# Configuration

Edit `mgs4_ultrawide.ini` beside `mgs4.exe` while the game is closed.

```ini
[Patch]
UltrawideEnabled=1
FPSOverrideEnabled=1

[Ultrawide]
Width=3440
Height=1440
FOVMultiplier=1.000
ConstrainUITo16x9=0

[FPS]
Limit=60
```

- `UltrawideEnabled=1` enables only the resolution, projection/FOV and optional
  UI hooks. Set it to `0` to leave the game's rendering path untouched.
- `FPSOverrideEnabled=1` enables only the selected FPS override. Set it to `0`
  to leave the game's original frame-rate behavior untouched.
- `Width` and `Height` set the render resolution and projection aspect ratio.
- `FOVMultiplier` scales the tangent of both FOV axes while preserving the
  configured aspect ratio. `1.000` retains the original vertical FOV; `1.150`
  is a visually validated wider comfort option. The accepted range is
  `0.500`-`2.000`.
- `Limit` accepts `30`, `60`, or `120`. Use 60 for gameplay. The 120 mode
  reproduced a scripted-intro stall and is included only for investigation.
- `ConstrainUITo16x9=0` leaves the game's original UI/full-screen-effect draws
  alone and is the stable default. Setting it to `1` centers identified D3D12
  UI draws in a 16:9 safe area, but is experimental because the same shader is
  used by some full-screen effects.

The log is written as `mgs4_ultrawide.log` beside the executable. A successful
D3D12 UI run contains messages for the projection hook, resolution hook,
D3D12 device/command-list hooks, and recognized UI shader.

Run `scripts/linux/configure.sh gui` or, on Windows,
`scripts\windows\configure.ps1` for the graphical configurator. It exposes
resolution, FOV, FPS, and UI settings. Linux also offers reversible
native/Gamescope Steam launch modes. Steam must be closed before changing its
launch options; the INI can still be saved while Steam is running.

The two modules are independent. For example, the `fps-only-120` profile sets
`UltrawideEnabled=0`, `FPSOverrideEnabled=1`, and `Limit=120`; no resolution,
projection, FOV, or UI hook is installed in that mode.

Ready-made command-line profiles are also available:

```bash
./scripts/linux/configure.sh stable     # 60 FPS, original UI
./scripts/linux/configure.sh ui-safe    # 60 FPS, centered UI experiment
./scripts/linux/configure.sh 120        # 120 FPS experiment, original UI
./scripts/linux/configure.sh fps-only-120 # original rendering, 120 FPS only
./scripts/linux/configure.sh ultrawide-only # ultrawide, original game FPS
```

On Windows, use `scripts\windows\configure.ps1 -Profile stable` (or
`ui-safe`, `120`, `120-ui`, `fps-only-120`, or `ultrawide-only`). The game must
be closed while changing a profile.

The patch intentionally overrides these values when `mgs4.exe` starts. The
corresponding internal game-menu settings may appear to save but will not take
effect while the patch is enabled. Uninstall the patch to restore full control
to the internal menu.

When loading a save, the dark Praying Mantis screen can remain visible after
loading has completed. If `PULSE CUALQUIER BOTÓN` is shown at bottom left,
press a button or click once; it is waiting for confirmation rather than stuck.
