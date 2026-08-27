# MGS4 Ultra 120 v0.2.1-alpha.1

This public alpha contains only the rendering and frame-rate patch.

## Included

- Native configurable ultrawide output without stretching a 16:9 image.
- Engine-level Hor+ projection correction shared by DirectX 11 and DirectX 12.
- Optional proportion-preserving FOV multiplier.
- Independent ultrawide/FOV and FPS override modules.
- Ready-made stable, ultrawide-only, 120 FPS, and 120-FPS-only profiles.
- Original UI as the stable default and an optional experimental centered
  16:9 D3D12 UI path.
- Graphical Windows and Linux configurators.
- Reversible installers and Linux native/Gamescope fullscreen profiles.
- Full source, build instructions, compatibility identity and technical notes.

## Validation

- 3440x1440 output with correct world proportions and additional horizontal
  view in cemetery, menu and saved-game scenes.
- Native 3440x1440 render surfaces without the repeated/corrupt right edge.
- FOV `1.000` and `1.150` compared at the same gameplay camera without changing
  geometry proportions.
- Stable profile exercised at 60 FPS through a completed save load into live
  gameplay.
- Installer/configurator syntax, reversible Steam option editing, platform
  packages and fail-closed executable validation checked locally.

## Alpha warnings

- 120 FPS is experimental. A scripted intro has stalled while audio continued.
- Centered 16:9 UI is experimental because some fullscreen effects reuse the
  same shader. Leave it disabled for the stable profile.
- A full playthrough is not yet certified.
- Unknown executable updates fail closed until their offsets are verified.

## SHA-256

```text
winmm.dll
9ffaa64eb656d10558cd900827f4ff13656b674e24e51d7c9d78e4f04c9e6f5a

MGS4Ultra120-v0.2.1-alpha.1-windows.zip
6ddec889bd324779ff15921b7b815fd9802baab1df3b80d8fb07e93bb1a4d4b1

MGS4Ultra120-v0.2.1-alpha.1-linux.tar.gz
0b6826b7ca87fe56f113555999da0399866085cdd3bbb2a1c403085389f45573
```
