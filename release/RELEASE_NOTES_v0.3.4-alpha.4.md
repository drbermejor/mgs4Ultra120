# MGS4 Ultra120 v0.3.4-alpha.4

This update improves the optional centered 16:9 HUD on ultrawide displays.
FOV, cinematic FOV, supersampling, FPS and controller behavior otherwise keep
the same defaults as alpha.3.

## What changed

- The HUD is now moved as one coherent 2D composition instead of correcting
  individual elements late in the frame.
- Menu text, subtitles, frames and panels no longer alternate between centered
  and stretched positions.
- The tested 3D weapon/item previews are fitted inside the centered HUD without
  changing their proportions, camera or FOV.
- The preview scissor state can no longer leak into a later rendering pass.
- If the fixed HUD-transform budget is ever exhausted, the HUD experiment
  disables itself cleanly for that run and records the reason in its log.

Windows testing at 3440x1440 covered gameplay, load screens, the weapon menu,
AK-102 and knife previews. The HUD remains experimental, DX12-only and disabled
by default. The same ASI is included for Linux/Proton, where broader visual
coverage is still needed.

## Install or update

Choose one download:

- **Windows setup EXE**: guided installation and reversible uninstall.
- **Windows portable ZIP**: extract it and run `MGS4Ultra120-Setup.cmd`.
- **Windows manual ZIP**: copy-only installation with no setup script.
- **Windows complete ZIP**: portable setup, manual files and documentation.
- **Windows 16:9 supersampling ZIP**: 4K-to-1080p preset without ultrawide or
  FOV changes.
- **Linux/Proton tar.gz**: extract it, exit Steam and run
  `./MGS4Ultra120-Linux-Setup.sh`.

Use the managed updater or remove the previous patch before copying the new
files. Do not mix the new `MGS4CenteredHUD16x9.asi` with an older HUD INI or ASI.

## Enabling and fallback

Enable **Experimental centered 16:9 HUD (DX12 only)** in the Windows or Linux
configurator. Leave it disabled if you want the established layout.

If a menu, prompt or fullscreen transition looks wrong, close the game and
disable only that HUD option. This restores the normal HUD without reinstalling
the patch or disabling ultrawide, FOV, FPS or controller fixes. Previous
releases, including the established `v0.3.1-alpha.5` renderer-only fallback,
remain available.

The Windows installer is unsigned. The portable and manual packages contain
the same patch and remain available for users who prefer to inspect every file.
