# MGS4 Ultra120 v0.3.4-alpha.2

This update fixes Linux installation when MGS4 is stored outside Steam's
default library.

## Linux fix

- Easy Setup now detects MGS4 through Steam's library records and app manifest.
- Native Steam, common Steam symlinks and Flatpak Steam are supported.
- If automatic detection cannot select one installation, a graphical folder
  picker asks for the folder containing `mgs4.exe`.
- The selected path is remembered for the desktop configurator and uninstaller.
- `MGS4_GAME_DIR` remains available as an advanced manual override.

The fix covers external drives, separate partitions, paths containing spaces
and Steam libraries whose game installation directory has a non-default name.

## Install

Choose one download:

- **Windows setup EXE**: guided installation and reversible uninstall.
- **Windows portable ZIP**: extract it and run `MGS4Ultra120-Setup.cmd`.
- **Windows manual ZIP**: transparent copy-only installation.
- **Windows complete ZIP**: portable setup, manual files and documentation.
- **Windows 16:9 supersampling ZIP**: portable setup preconfigured for
  1920x1080 output and a 3840x2160 internal render, without ultrawide or FOV
  changes.
- **Linux/Proton tar.gz**: extract it, exit Steam and run
  `./MGS4Ultra120-Linux-Setup.sh`.

The normal Windows rendering behavior is unchanged from `v0.3.4-alpha.1`; this
release adds the explicit 16:9 preset and package. The cinematic FOV and
centered 16:9 HUD options remain experimental and disabled by default.

The 16:9 preset keeps the controller fix and optional corrected FPS/launcher
features available. Its 3840-pixel internal width is below the observed 4096
crosshair boundary, but supersampling remains experimental and can sharply
reduce performance or exhaust VRAM.

The Windows EXE remains unsigned. It is optional; use the manual ZIP if you
prefer to inspect and copy the open-source files yourself.

Do not mix ASIs, INIs or proxy DLLs from different releases. Completely remove
an older build before installing this one.

## Manual Linux override

Automatic discovery or the graphical picker should normally be sufficient. A
terminal user can still specify the folder directly:

```bash
MGS4_GAME_DIR="/path/to/METAL GEAR SOLID 4/MGS4" \
  ./MGS4Ultra120-Linux-Setup.sh
```

The folder must contain `mgs4.exe`.

Reported and tracked in
[`#6`](https://github.com/drbermejor/mgs4Ultra120/issues/6).
