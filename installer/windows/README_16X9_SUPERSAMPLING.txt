MGS4 Ultra120 - 16:9 4K-to-1080p supersampling preset
======================================================

This portable package is for a 1920x1080 16:9 display. It starts with:

- ultrawide correction disabled;
- gameplay and cinematic FOV changes disabled;
- centered-HUD experiment disabled;
- 1920x1080 physical output;
- 2.00x supersampling (3840x2160 internal render);
- controller-profile correction disabled by default (opt in only if needed);
- optional launcher bypass enabled;
- optional corrected 30/60/120 FPS available through Easy Setup.

It does not require AMD VSR, NVIDIA DSR or a 4K desktop resolution.

How to install
--------------

1. Close the game.
2. Extract this ZIP. Do not run it from inside the compressed archive.
3. Run MGS4Ultra120-Setup.cmd.
4. Choose whether to install corrected FPS support.
5. Confirm the folder containing mgs4.exe.
6. In the configurator, review the preselected 16:9 settings and save.
7. Launch normally through Steam.

The same preset is available later through the "16:9 4K -> 1080p preset"
button in the Windows configurator.

Important
---------

Supersampling is experimental and can reduce performance or exhaust VRAM.
The previous aiming-reticle failure at internal widths of 4096 pixels or more
is fixed in v0.3.4-alpha.6. This preset uses a 3840x2160 internal render.

Higher scales are no longer restricted by the old reticle boundary, but they
increase GPU load and VRAM use quickly. Disable supersampling if the game,
presentation path or overlays behave incorrectly.

Leave the controller-profile correction disabled when using keyboard/mouse or
a controller together with mouse/gyro aiming. Enable it only if the game is
selecting the wrong native controller profile on your system.

This is the normal open-source MGS4 Ultra120 package with a different default
profile, not a separate patch or modified game executable.
