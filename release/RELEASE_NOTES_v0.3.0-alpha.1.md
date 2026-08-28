# MGS4 Ultra120 v0.3.0-alpha.1

This alpha turns the patch into a set of independently selectable modules.

## Included

- Native configurable ultrawide output and engine-level Hor+ projection for
  the common DirectX 11/DirectX 12 path.
- Optional proportion-preserving FOV multiplier.
- Independent 30/60/experimental 120 FPS override.
- Configurable in-game hotkey, F10 by default, to switch 60/120 without a
  polling loop.
- Independent controller-profile correction using the game's native detected
  controller, with no virtual controller or system driver.
- Optional wrapper that skips the Unity launcher while retaining Steam as the
  launch path.
- Reversible Windows and Linux installers/configurators.
- Original UI as the stable default and a clearly labelled experimental D3D12
  16:9 safe-area prototype.
- Explicit unsafe override for unknown executables; they remain blocked by
  default and local hook signatures are still checked.

## Validation

- 3440x1440 world rendering has correct proportions and additional horizontal
  view in real-time cemetery, menu and saved-game scenes.
- Native render surfaces eliminate the previous repeated/corrupt right edge.
- Controller disconnect/reconnect releases and reacquires the native profile
  under Proton without restarting the game.
- The direct wrapper starts the game through the Steam launcher path and
  returns the child process exit code.
- Windows and Linux packages contain the DLL, wrapper, default configuration,
  reversible scripts, documentation and full redistributable source.

## Alpha warnings

- 120 FPS has reproduced a scripted-scene stall while audio continued. Return
  to 60 before cinematics/scripts; physics and QTE timing are not certified.
- The D3D12 16:9 UI prototype can also constrain full-screen effects.
- Pre-rendered Bink output is not modified. Forced pillarboxing remains under
  investigation rather than using an unsafe draw heuristic.
- Unknown-build override can crash because data RVAs may have moved.
- Full-playthrough, broader Windows controller and Steam Cloud transfer tests
  remain outstanding.

## SHA-256

```text
winmm.dll
c3b28d0307ef4df2e029930168ff691224923a7f84f8cc6dd5522cdf002c7d9b

launcher.exe
fc0b443a1a3105f361cc6d558989e709320b60f41a350735dd994e6f56c6fe23

MGS4Ultra120-v0.3.0-alpha.1-windows.zip
ed8dfbb099253250f34f4b1a443ae806e946ddb5b8733717a8d4d47fad74a1b4

MGS4Ultra120-v0.3.0-alpha.1-linux.tar.gz
ed6598e18e11a3bf5caeb713afe7e52dcac29cd7fc10c27ba48c141e1bc09806
```
