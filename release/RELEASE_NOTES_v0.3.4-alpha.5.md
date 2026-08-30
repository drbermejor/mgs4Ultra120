# MGS4 Ultra120 v0.3.4-alpha.5

This release fixes the aiming reticle at high internal render resolutions and
makes the Windows/Linux setup safer. Thank you to
[@ngsalmon](https://github.com/ngsalmon) for identifying and contributing the
core reticle fix in [PR #8](https://github.com/drbermejor/mgs4Ultra120/pull/8).

## Reticle and supersampling

- Removed the signed 16-bit truncation from all four matching X/Y reticle
  coordinate routes.
- The four instruction sites are validated together before anything is changed;
  unknown or partly modified code is left untouched.
- Native Windows gameplay validation passed at 3440x1440 output with 1.50x
  supersampling (5160x2160 internal), where the reticle had previously
  disappeared.
- The old recommendation to keep internal width below 4096 is obsolete. The
  supersampling option remains experimental because GPU load, VRAM use and
  presentation/overlay scaling still vary by system.

## Linux and configurator improvements

- Custom Steam-library paths can now be passed directly:

  ```bash
  ./MGS4Ultra120-Linux-Setup.sh "/path/to/METAL GEAR SOLID 4/MGS4"
  ```

- Running setup while the current directory contains `mgs4.exe` is also
  supported. Editing or copying internal scripts is no longer necessary.
- Managed setup records the installed version, replaces its managed shortcut
  and rejects a mismatched/stale configurator.
- The controller-profile workaround is now opt-in. Keep it disabled for
  keyboard/mouse or hybrid controller plus mouse/gyro input. Managed updates
  reset it to the safe disabled default. **If you previously needed this fix,
  you must enable `Controller profile fix` again in the configurator and save.**
  Manual users can set `ControllerProfileFixEnabled=1` under `[Input]` in
  `mgs4_ultrawide.ini`.

## Important centered-HUD warning

The optional centered 16:9 HUD is a **known-broken development preview**. Menus,
subtitles, maps, text and 3D inventory previews can be misplaced, squashed,
clipped or flicker. It remains disabled by default and should stay disabled for
normal play. Set `Enabled=0` in `mgs4_centered_hud_16x9.ini` to restore the
normal HUD without disabling ultrawide, FOV, FPS, supersampling or the reticle
fix. Managed setup resets a previous HUD opt-in to `Enabled=0`; testers must
deliberately enable it again after reading the current warning.

## Downloads

- Windows setup EXE
- Windows portable, manual, complete and 16:9 supersampling ZIPs
- Linux/Proton tar.gz

The Windows installer is unsigned. Download only from this official repository
and keep normal security protections enabled.
