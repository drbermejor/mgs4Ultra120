# MGS4 Ultra120 v0.3.4-alpha.3

This update fixes a runtime stability problem in the optional centered 16:9
HUD. All other patch features and defaults remain the same as alpha.2.

## What changed

The experimental centered HUD could briefly stretch back to the full ultrawide
layout and then become centered again, especially around the dynamic weapon or
item panels after playing for a while.

The HUD resource tracker now:

- ignores unrelated D3D12 textures;
- preserves dynamic UI-buffer updates at their correct offsets;
- safely recycles stale resource entries without discarding active HUD data;
- handles reused D3D12 resource and pipeline identities.

The corrected Windows build was tested at 3440x1440 with repeated gameplay,
weapon/item HUD and pause-menu use beyond the previous failure interval. The
stretch/shrink pulse did not return.

## Install or update

Choose one download:

- **Windows setup EXE**: guided installation and reversible uninstall.
- **Windows portable ZIP**: extract it and run `MGS4Ultra120-Setup.cmd`.
- **Windows manual ZIP**: copy-only installation for users who prefer to inspect
  every file.
- **Windows complete ZIP**: portable setup, manual files and documentation.
- **Windows 16:9 supersampling ZIP**: preconfigured 4K-to-1080p supersampling
  without ultrawide or FOV changes.
- **Linux/Proton tar.gz**: extract it, exit Steam and run
  `./MGS4Ultra120-Linux-Setup.sh`.

Remove older patch files before installing, or use the managed setup/update
route. Do not mix ASIs, INIs or proxy DLLs from different releases.

## Experimental HUD status

Centered 16:9 HUD remains experimental, DX12-only and disabled by default. It
can be enabled in the configurator. If any UI or full-screen effect behaves
incorrectly, disable only the centered-HUD option to return to the normal layout
without reinstalling the rest of the patch.

The same HUD ASI is included in Windows and Linux/Proton packages. The runtime
fix was visually validated on Windows; a repeat Linux/Proton visual pass is
still recommended because automated tests cannot verify final presentation.

The unsigned Windows EXE remains optional. Use the portable or manual package
if you prefer to inspect the open-source scripts and copy the files yourself.
