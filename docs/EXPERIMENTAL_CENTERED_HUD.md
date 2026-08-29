# Experimental centered 16:9 HUD

This optional DX12-only companion keeps accepted HUD draws inside a centered
16:9 safe area on ultrawide output. It is included on Windows and Linux/Proton
but disabled by default.

Use the **Experimental centered 16:9 HUD** switch in either configurator. For a
manual installation, close the game and edit:

```ini
; mgs4_centered_hud_16x9.ini
[Lab]
Enabled=1
```

Set `Enabled=0` to return to the normal HUD. The companion ASI may stay in the
`scripts` folder: while disabled, it exits before installing D3D12 hooks and
does not alter FOV, resolution, input, FPS or launcher behavior.

The current classifier deliberately leaves unknown draws, full-screen bounds,
large single-quad effects and indexed UI untouched. Gameplay and the tested
weapon window look correct, but less common save, pause, inventory, Codec,
subtitle or prompt layouts may still need refinement. If text separates from a
panel, a menu clips or a full-screen effect is reduced to a central strip,
close the game and disable the option before reporting the scene and resolution.

This feature does not fix the separate high-internal-resolution crosshair
boundary associated with supersampling widths around or above 4096 pixels.
