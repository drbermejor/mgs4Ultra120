# MGS4 Ultra120 v0.3.3-alpha.1 - single-owner experimental FOV

This alpha isolates the FOV multiplier to the primary native camera route.
Earlier builds applied it again during downstream camera rebuilds, which could
distort the WeaponWindow and made values such as `1.050` inconsistent.

## What changed

- Route `0x0ba3a3` is now the sole native-FOV owner. Downstream rebuild routes
  remain untouched.
- Windows testing at 3440x1440 and `FOVMultiplier=1.200` passed gameplay, the
  WeaponWindow and the previously problematic in-engine cinematics without the
  observed distortion, narrowing or culling regression.
- `1.200` is the tested 21:9 recommendation and supported maximum under this
  corrected model. `1.000` keeps the game's original vertical FOV.
- Native FOV remains explicitly **experimental**. It can be disabled separately
  in either configurator; ultrawide aspect correction remains enabled with the
  original vertical FOV. Disable native FOV first if instability or a new scene
  regression appears.
- Managed Windows updates migrate the old repeated-route default `1.050` to
  model-2 `1.200` once, then preserve later user choices and an explicit opt-out.
- Setup now blocks known duplicate Ultimate ASI Loader proxies and renamed old
  MGS4 Ultra120 ASIs. It lists conflicts and never deletes third-party DLLs.
- The language selector now preserves the chosen language. The direct wrapper
  reproduces the official Unity launcher's child process; Spanish was verified
  in game without reading or modifying saves.
- Optional supersampling remains experimental and disabled by default.

## Legacy fallback

`v0.3.1-alpha.5` remains available as the previously validated renderer-only
fallback. First try disabling **Experimental native FOV** in this release if a
scene-specific regression appears. If you return to alpha.5, completely remove
the current build first; never combine files from the two versions.

## FOV limitation

Any FOV above `1.000` can reveal actors, geometry or animation transitions near
the edges before the original shot intended them to enter. That authored
staging limitation can remain even when projection and culling are correct.

## Remove old builds before installing

Do not combine files from different MGS4 Ultra120 releases. Uninstall the old
guided package first, or remove old patch/loader copies after backing up files
owned by other mods. The supported layout contains one loader and one patch ASI:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
```

Do not also keep Ultimate ASI Loader as `dinput8.dll`, `d3d11.dll`,
`wininet.dll`, `winhttp.dll` or another proxy name. Do not keep renamed or old
MGS4 Ultra120 ASIs. Guided setup detects these known conflicts and stops with a
file list rather than guessing what it may delete.

The manual ZIP does not install the optional Unity-launcher bypass. Launch
through Steam and select the game language in the official Unity launcher.

## Downloads

- `MGS4Ultra120-v0.3.3-alpha.1-windows-setup.exe` - guided setup, conflict
  detection, configurator, shortcuts and reversible uninstall; unsigned.
- `MGS4Ultra120-v0.3.3-alpha.1-windows-portable.zip` - guided setup via CMD.
- `MGS4Ultra120-v0.3.3-alpha.1-windows-manual.zip` - copy-only payload.
- `MGS4Ultra120-v0.3.3-alpha.1-windows-complete.zip` - portable setup, manual
  folder, documentation and notices.
- `MGS4Ultra120-v0.3.3-alpha.1-linux.tar.gz` - Linux/Proton setup and
  configurator. It uses the same route policy; broader Proton scene validation
  is still requested for this exact build.

MGSFPSUnlock is not redistributed. Guided setup fetches its official 0.1.0
release and verifies pinned hashes. Supersampling users should keep internal
width below 4096 for a stable aiming reticle.

The final packaged Windows build completed a clean real-machine install and
Steam start. Its runtime log reported native FOV active, 72 primary-route input
adjustments, 656 aspect-only renderer corrections, zero corrections beyond the
old scale ceiling, no native-hook fallback, Spanish parser id 5 and no startup
error. Windows complete/portable package
smokes and the Linux install-update-uninstall smoke passed. Microsoft Defender
reported no new detections after scanning the final five release assets.

The installed release build was then confirmed visually in gameplay, the
WeaponWindow and the previously problematic in-engine cinematic. No distortion,
shrinking or new culling problem was observed in that final gate.

Download only from the official GitHub release. The project is open source,
contains no game files and never modifies `mgs4.exe`.
