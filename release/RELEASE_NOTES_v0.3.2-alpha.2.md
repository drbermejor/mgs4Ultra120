# MGS4 Ultra120 v0.3.2-alpha.2 - native FOV release

This release unifies Windows and Linux around the definitive native-camera FOV
implementation. FOV is now applied to the game's original camera input before
MGS4 builds its projection, combined matrices and culling frustum. The final
renderer hook corrects aspect only, so FOV is applied exactly once.

## What changed

- Fixed cinematic FOV continuity without the tall/thin character regression
  from the withdrawn post-return alpha.6 experiment.
- Removed the old conservative projection-scale ceiling, so tight close-ups
  stay on the corrected path.
- Fixed configurator language persistence. The chosen language is now written
  to the patch INI, normalized for direct launch and synchronized with the
  official Unity launcher's saved state.
- Corrected German from the invalid legacy token `ge` to the game's `gr` token
  and added Portuguese (`pt`). Existing `ge` settings migrate automatically.
- Kept optional supersampling in the unified packages. It remains experimental
  and disabled by default.

## Native validation and final FOV range

The final Windows candidate was tested at:

- 3440x1440 physical output;
- final `FOVMultiplier=1.050` framing;
- corrected 120 FPS through MGSFPSUnlock.

Runtime logging confirmed that native mode remained active with no fallback or
new crash. Natural object proportions, the `1.050` framing and the in-game
cinematic fix were confirmed visually.

`FOVMultiplier=1.050` is the recommended 21:9 default and supported maximum.
`1.000` preserves the original vertical FOV, but final visual comparison found
that it framed Snake too tightly in the tested gameplay view. Values above
`1.000` may reveal actors, geometry, animation transitions or other off-camera
content before a cutscene was authored to show it. Projection and culling can
be correct while this staging limitation is still visible; both configurators
warn when saving a value above `1.000`.

All application-facing components identify themselves as
`v0.3.2-alpha.2`, including Windows file properties for the ASI and launcher,
GUI titles, runtime logs, the installed-app display name and Linux shortcuts.

## Downloads

- `MGS4Ultra120-v0.3.2-alpha.2-windows-setup.exe` - guided Windows setup,
  shortcuts, configurator and reversible uninstall. This EXE is unsigned.
- `MGS4Ultra120-v0.3.2-alpha.2-windows-portable.zip` - the same guided setup
  through `MGS4Ultra120-Setup.cmd`, without installing a persistent app.
- `MGS4Ultra120-v0.3.2-alpha.2-windows-manual.zip` - minimal transparent
  copy-only loader, INI and ASI. Copy its contents beside `mgs4.exe`.
- `MGS4Ultra120-v0.3.2-alpha.2-windows-complete.zip` - portable setup, manual
  folder, documentation and notices together.
- `MGS4Ultra120-v0.3.2-alpha.2-linux.tar.gz` - Linux/Proton setup,
  configurator and reversible uninstall.

MGSFPSUnlock is not redistributed. Guided setup fetches its official 0.1.0
release and verifies the pinned hashes; manual users can copy its ASI and INI
from the upstream release into `MGS4/scripts`.

Supersampling users should keep internal width below 4096 for normal aiming:
the reticle is stable at 3956, can flicker at 4096 and may disappear above that
according to aiming depth.

Download only from this official GitHub release. The source, build scripts and
installer definition are public and can be inspected or built independently.
No game files are included and `mgs4.exe` is never modified.

The five published assets were scanned locally with Microsoft Defender after
the final build and produced no detections. The setup EXE remains unsigned.
