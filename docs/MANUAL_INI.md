# Manual DLL and INI installation

The Windows ZIP contains a ready-to-drag `Manual-Install` folder with:

- `winmm.dll`
- `mgs4_ultrawide.ini`
- `scripts/MGS4Ultra120.asi`

Close the game. Drag everything **inside** `Manual-Install` into the folder
containing `mgs4.exe` (normally
`...steamapps\common\METAL GEAR SOLID 4\MGS4`). Keep the included `scripts`
folder so the ASI lands at `MGS4\scripts\MGS4Ultra120.asi`. Then start the game
through Steam. The patch never replaces `mgs4.exe`.

This copy-only route does not enable the direct-launch wrapper. To skip Unity,
use the GUI/CMD setup once or follow the separately documented reversible
launcher procedure. `winmm.dll` is pinned Ultimate ASI Loader `v9.7.4`; if a
compatible x64 Ultimate ASI Loader already exists under another supported proxy
name, use only one loader and follow that mod's plugin instructions. Do not
overwrite an unknown proxy without a backup.

## Editing the shared configuration

`mgs4_ultrawide.ini` beside `mgs4.exe` is the single source of settings. Edit
it only while the game is closed. The Windows GUI reads and updates this same
file, preserves supported values during package updates and can be used again
after manual edits.

Accepted values:

| Key | Values |
|---|---|
| `UltrawideEnabled` | `0` or `1` |
| `FPSOverrideEnabled` | `0` or `1` |
| `AllowUnsupportedExecutable` | `0` recommended; `1` is unsafe |
| `Width`, `Height` | physical output size, 640–16384 / 480–16384 |
| `FOVMultiplier` | `0.500`–`2.000`; `1.000` retains vertical FOV |
| `ConstrainUITo16x9` | `0` stable; `1` experimental D3D12 mode |
| `Limit` | `30`, `60`, or experimental `120` |
| `ToggleHotkey` | `Off`, `F1`–`F24`, one letter, or one digit |
| `ToggleHotkeyModifiers` | `None` or `Ctrl+Alt+Shift+Win` subsets |
| `ControllerProfileFixEnabled` | `0` or `1` |
| `SkipUnityLauncher` | `0` or `1`; applied by GUI/CMD setup |
| `Language` | launcher token such as `en`, `sp`, `fr`, `ge`, `it`, `jp` |
| `DisplayMode` | Windows: `Windowed` recommended or `Fullscreen` advanced |
| `UsePrimaryPhysicalResolution` | Windows: `1` detects the physical primary size in the GUI; `0` keeps manual dimensions |

The stable defaults are native ultrawide, FOV `1.000`, 60 FPS, original UI,
controller-profile correction enabled and Unity bypass selected. Keyboard and
mouse users should set `ControllerProfileFixEnabled=0`.

For manual Windows use, start from the INI shipped inside the Windows release
ZIP. It already contains the two Windows presentation keys. Editing the INI
alone cannot safely synchronize or back up the official `launcher_sv` values;
open the configurator and click **Save settings** once when changing
presentation mode. Do not map `DisplayMode` to the wrapper's `-resolution`
token: native tracing shows that token remains slot `0` in both modes.

To remove a purely manual Windows installation, close the game and remove only
`scripts/MGS4Ultra120.asi`, `mgs4_ultrawide.ini`, and the `winmm.dll` you
personally copied. Do not remove `winmm.dll` when it was already used by
another ASI mod. Restore your own backups for every pre-existing filename.
