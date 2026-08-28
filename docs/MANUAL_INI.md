# Manual DLL and INI installation

The portable manual route uses the same two files as the graphical installer:

- `bin/winmm.dll`
- `config/mgs4_ultrawide.ini`

Close the game, copy both files into the folder containing `mgs4.exe` (normally
`...steamapps\common\METAL GEAR SOLID 4\MGS4`) and start the game through
Steam. The patch never replaces `mgs4.exe`.

This copy-only route does not enable the direct-launch wrapper. To skip Unity,
use the GUI/CMD setup once or follow the separately documented reversible
launcher procedure. Do not overwrite an existing `winmm.dll` from another mod
without backing it up; DLL proxy mods at the same filename must be combined by
their authors rather than stacked.

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

To remove a purely manual installation, close the game and remove only the
`winmm.dll` and `mgs4_ultrawide.ini` that you copied. If either filename
existed before installation, restore your own backup instead.
