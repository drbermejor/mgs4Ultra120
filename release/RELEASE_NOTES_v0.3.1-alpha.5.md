# MGS4 Ultra120 v0.3.1-alpha.5 - refreshed Windows and Linux release

This release was refreshed in place so previously shared alpha.5 links remain
valid. **Redownload the asset you use because older alpha.5 downloads were
replaced.** Linux remains a separate Proton release line; use the alpha.5 Linux
tarball for the corrected current scripts.

The latest refresh synchronizes the expanded FOV with the game's CPU visibility
frustum. Earlier alpha.5 downloads used a projection-only adjustment and are
obsolete even though the filenames are unchanged.

## What it does

MGS4 Ultra120 provides native ultrawide/Hor+ rendering, configurable FOV, a
controller-profile correction and an optional reversible Unity-launcher bypass
for the Steam PC version of *METAL GEAR SOLID 4*.

Corrected 120 FPS support is now available as an optional additive component
through [cipherxof/MGSFPSUnlock 0.1.0](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0).
Thank you to cipherxof for the substantially improved FPS unlock and its
camera, character, cutscene, physics, ragdoll, cloth, hair and wind timing work.
MGS4 Ultra120's old single-value FPS writer is disabled so the two plugins do
not compete.

## Keep-it-simple installation choices

### Setup EXE

Automatic game detection, configuration, shortcuts and reversible uninstall.
The optional **Install / update improved 120 FPS support** box is checked by
default. It downloads the official MGSFPSUnlock ZIP and requires pinned archive
and ASI SHA-256 values to match. Uncheck it for the offline core/normal-FPS
route. Unchecking only skips installation/update; it does not remove an
existing copy.

The EXE is unsigned and may receive a browser or SmartScreen reputation warning.
It is optional; the complete source and build instructions are public.

### Portable ZIP

Extract it and run `MGS4Ultra120-Setup.cmd`. It exposes the same core-versus-120
choice without registering a persistent Windows application. Keep the folder
for later configuration or uninstall.

### Manual ZIP - no installer or scripts

Extract it and copy everything into the `MGS4` folder containing `mgs4.exe`:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
```

For corrected 120 FPS, separately download the official
[MGSFPSUnlock 0.1.0 ZIP](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0)
and copy only these files from its `MGSFPSUnlock\scripts` folder:

```text
MGS4\scripts\MGSFPSUnlock.asi
MGS4\scripts\MGSFPSUnlock.ini
```

Do not copy its `d3d11.dll`, `winhttp.dll` or `wininet.dll`; this release already
provides Ultimate ASI Loader as `winmm.dll`. Confirm the external INI contains
`TargetFrameRate = 120`.

### Complete ZIP

Contains the portable route, a ready-to-copy `Manual-Install` folder and all
documentation, licences and notices. Choose one installation route.

### Linux core tarball

`MGS4Ultra120-v0.3.1-alpha.5-linux.tar.gz` is the current separate Proton
package. It fixes the unconditional `gamemoderun` launch failure and does not
write Gamescope options when Gamescope is missing. It provides ultrawide/FOV,
controller correction and launcher choice using the game's normal FPS behavior.
The external corrected-120 ASI route remains Windows-only until tested under
Proton.

## Fixes in this refresh

- Applies the ultrawide/FOV correction in the central camera builder and
  rebuilds both view-projection matrices and all six normalized visibility
  planes. This fixes the former render-FOV/culling mismatch and side pop-in
  caused by the earlier projection-only path.
- Uses `FOVMultiplier=1.150` as the recommended 21:9 framing. `1.000` remains
  available for users who prefer the original vertical FOV.
- Prevents the renderer fallback from applying the FOV multiplier twice to an
  already corrected target-aspect matrix.
- Adds projection/frustum unit tests and runtime counters. Native 3440x1440
  testing completed hundreds of synchronized camera/frustum builds without a
  late fallback or crash, and the aiming crosshair remained visible.
- Preserves the working core/normal-FPS route; improved 120 is additive and
  optional rather than a hard dependency.
- Removes the unverified UI/safe-area experiment from the release binary and
  configurator. Original HUD/menu/effect behavior is retained.
- Ignores stale Steam libraries on disconnected drives instead of aborting with
  `Cannot find drive E:`/`H:`.
- Parses manual FOV values independently of Windows locale and records the exact
  INI path/values if configuration is rejected.
- Accepts official launcher settings that omit redundant `WindowSizeW/H`
  fields and reports GUI save failures accurately.
- Keeps the validated x64 Ultimate ASI Loader v9.7.4 layout and safe ownership
  backups/uninstall.
- Keeps the post-alpha.2 direct-launch wrapper enabled by default. It addresses
  the repeated Steam custom-arguments loop reported in
  [Issue #2](https://github.com/drbermejor/mgs4Ultra120/issues/2) by passing
  launch data through `mgs4_param`; the issue itself was closed at alpha.3
  without a final reporter confirmation, so this refresh relies on local and
  automated wrapper validation rather than claiming their confirmation.

## Known limitations

- 3440x1440 with FOV 1.150 is the validated ultrawide target. The synchronized
  camera/frustum logic is resolution-independent, but an external 5120x2160
  report of a missing aiming crosshair has not yet been reproduced at that exact
  resolution and therefore is not claimed fixed.
- A full playthrough at corrected 120 FPS has not yet been certified by this
  project. Include both plugin logs when reporting timing issues.
- On one mixed-refresh NVIDIA multi-monitor Windows setup, G-SYNC/VRR focus
  changes caused red sweep/flicker; the configurator warns but changes no system
  setting.
- The refreshed Linux scripts have not yet received an end-to-end CachyOS
  retest. KDE/Wayland users can try `Super+F` when the desktop panel remains
  visible or return to the native launch option.

Download only from this official release, keep antivirus enabled and avoid
copies reuploaded by third parties.
