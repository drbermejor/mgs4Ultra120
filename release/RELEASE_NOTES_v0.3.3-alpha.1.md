# MGS4 Ultra120 v0.3.3-alpha.1

Recommended current alpha for Windows and Linux/Proton. This version fixes the
FOV distortion seen in the weapon menu and keeps ultrawide rendering, corrected
high-frame-rate support, controller fixes and the optional launcher bypass.

The final Windows package was tested at 3440x1440 in gameplay, the weapon menu
and the previously problematic in-engine cinematics.

## Before installing

Completely remove any older MGS4 Ultra120 build. Do not combine files from
different releases or keep a second ASI loader under another DLL name. Guided
setup detects known conflicting loaders and old project ASIs.

## Recommended settings

- **Most stable:** leave **Experimental native FOV** disabled. Ultrawide aspect
  correction still works with the game's original vertical FOV.
- **Expanded 21:9 FOV:** enable it and use `1.200`. Do not exceed `1.200` at
  21:9. Disable the option again if a game scene behaves incorrectly.
- **Supersampling:** experimental and disabled by default. Keep internal width
  below 4096 to avoid aiming-reticle problems.

## Choose one download

- **Windows Setup EXE:** easiest guided installation, configuration, shortcuts
  and uninstall. The executable is unsigned.
- **Windows Portable ZIP:** extract it and run `MGS4Ultra120-Setup.cmd`.
- **Windows Manual ZIP:** copy-only package for users who do not want to run the
  installer or scripts.
- **Windows Complete ZIP:** portable setup, manual files and documentation.
- **Linux package:** installer and configurator for Linux/Proton.

## Manual Windows installation

1. Close the game and remove the previous patch.
2. Extract the Manual ZIP.
3. Copy everything into the folder that directly contains `mgs4.exe`.
4. Launch the game through Steam and use the official launcher to choose the
   language.

The final layout is:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
```

For corrected 120 FPS, manual users must also copy `MGSFPSUnlock.asi` and
`MGSFPSUnlock.ini` from the official
[MGSFPSUnlock 0.1.0 release](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0)
into `MGS4\scripts`. Guided setup downloads and verifies these files
automatically.

## Legacy fallback

`v0.3.1-alpha.5` remains available as the previously validated renderer-only
fallback. Never install it over this version: remove the current build first.

## Security notice

The Setup EXE is not digitally signed, so Windows or a browser may show an
unknown-publisher warning. The installer is optional: use the Portable or
Manual ZIP if preferred. The project is open source, contains no game files and
never modifies `mgs4.exe`.

[Gameplay screenshot](https://github.com/drbermejor/mgs4Ultra120/blob/main/docs/images/v0.3.3-alpha.1-gameplay-3440x1440.png) ·
[Weapon-menu screenshot](https://github.com/drbermejor/mgs4Ultra120/blob/main/docs/images/v0.3.3-alpha.1-weapon-window-3440x1440.png)
