# MGS4 Ultra120 v0.3.2-alpha.1 — unified Linux / Proton setup

This prerelease brings the Windows plugin architecture and guided feature set
to Linux/Proton. The Linux package now uses pinned Ultimate ASI Loader 9.7.4,
`MGS4Ultra120.asi` and optional MGSFPSUnlock 0.1.0 as separate modules.

## Easy Linux setup

1. Download `MGS4Ultra120-v0.3.2-alpha.1-linux.tar.gz` and extract it.
2. Exit Steam completely.
3. Run `./MGS4Ultra120-Linux-Setup.sh` from a terminal in the extracted folder.
4. Choose whether to install corrected FPS support, then configure resolution,
   FOV, supersampling, FPS, launcher bypass and fullscreen mode in the GUI.
5. Start Steam and launch MGS4 using DirectX 12.

Setup keeps a stable local copy under `~/.local/share/mgs4Ultra120` and creates a
configurator shortcut on the desktop and in the application menu.

MGSFPSUnlock is downloaded directly from its official release because upstream
does not declare a redistribution license. Setup verifies the upstream archive
and ASI hashes, applies the Wine compatibility byte locally and verifies the
result. The release asset does not contain MGSFPSUnlock or any game file.

## Proton validation

Validated with GE-Proton10-34 at 3440x1440:

- native Hor+ rendering and FOV 1.150;
- 1.15x supersampling at 3956x1656 internal resolution;
- exact 3440x1440 Gamescope client and outer window;
- launcher bypass through Steam;
- Ultimate ASI Loader loading both independent plugins;
- corrected 120 FPS target plus camera, character, polygon-demo, wind, SPURS,
  hair, cloth, rigid-body and ragdoll timing hooks on three consecutive starts.

## Important warnings

- 120 FPS and supersampling remain experimental.
- A complete 120 FPS playthrough is not yet certified. Report stalled scripted
  scenes, QTE timing or physics differences and switch to 60 FPS when needed.
- Keep internal supersampling width below 4096 to avoid the known reticle issue.
- The unfinished UI/safe-area experiment is not included.
- Only the currently supported Steam executable is verified. The unsafe
  override remains available under the user's responsibility.
