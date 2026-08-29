MGS4 Ultra120 - 16:9 4K-to-1080p supersampling preset
======================================================

This portable package is for a 1920x1080 16:9 display. It starts with:

- ultrawide correction disabled;
- gameplay and cinematic FOV changes disabled;
- centered-HUD experiment disabled;
- 1920x1080 physical output;
- 2.00x supersampling (3840x2160 internal render);
- controller-profile correction enabled;
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
The known crosshair problem begins around an internal width of 4096 pixels;
this preset uses 3840 pixels and stays below that observed boundary.

Do not increase the output resolution or render scale without checking the
calculated internal width. Disable supersampling if the game, overlay or aiming
reticle behaves incorrectly.

This is the normal open-source MGS4 Ultra120 package with a different default
profile, not a separate patch or modified game executable.
