# Experimental supersampling

This optional feature is included in `v0.3.4-alpha.4`, but remains experimental
and disabled by default. The single-owner experimental FOV route does not depend on
supersampling; users who do not need downsampling should leave it off.

Supersampling keeps the configured `Width` and `Height` as the physical Windows
output and game-window size, while a uniform `RenderScale` raises the internal
render resolution. DXGI then presents that larger frame at the physical output
size. It does not require AMD VSR, NVIDIA DSR or changing the Windows desktop
resolution.

Examples:

| Physical output | Scale | Internal render |
| --- | ---: | --- |
| 1920x1080 | 2.00 | 3840x2160 |
| 2560x1080 | 2.00 | 5120x2160 |
| 3440x1440 | 1.50 | 5160x2160 |
| 3440x1440 | 2.00 | 6880x2880 |

Use the Windows configurator or add this section to `mgs4_ultrawide.ini`:

```ini
[Supersampling]
SupersamplingEnabled=1
RenderScale=1.50
```

`RenderScale=1.00` performs no supersampling. Uniform scaling preserves the
configured aspect ratio, so the existing ultrawide projection/FOV correction
continues to use the physical output aspect.

## 16:9 4K-to-1080p preset

`v0.3.4-alpha.4` includes a separate
`windows-16x9-supersampling.zip` portable package and a
**16:9 4K -> 1080p preset** button in the normal Windows configurator. The same
profile is available from the command line on Windows and Linux.

The preset uses:

```ini
UltrawideEnabled=0
Width=1920
Height=1080
FOVMultiplier=1.000
NativeCameraFOV=0
ExperimentalCinematicFOV=0
SupersamplingEnabled=1
RenderScale=2.000
```

It renders at 3840x2160 and presents at 1920x1080. The controller-profile fix
remains enabled, while corrected 30/60/120 FPS and the launcher bypass remain
available through Easy Setup. AMD VSR, NVIDIA DSR and a 4K desktop mode are not
required.

Supersampling remains experimental and its GPU/VRAM costs are the user's
responsibility. The earlier 4096-pixel aiming-reticle boundary has been removed
from the native X/Y coordinate routes.

## Warnings

- Rendering cost and approximate render-target memory grow with the square of
  the scale: 1.50x uses 2.25x as many pixels and 2.00x uses 4x.
- Excessive values can exhaust VRAM, reduce performance, crash the game or reset
  the graphics driver. The patch reports unusually large requests but does not
  guess a safe GPU-specific limit.
- The whole rendered frame is reduced to the output size. HUD text and the Steam
  overlay may therefore appear smaller; supersampling does not yet separate
  world rendering from UI composition.
- Windowed presentation is the initial test target. Exclusive fullscreen,
  HDR, VRR/G-SYNC and mixed-refresh multi-monitor combinations require separate
  validation.
- Close the game before changing the setting. If startup fails, set
  `SupersamplingEnabled=0` manually to return to the stable path.

The configurator displays the calculated internal resolution and requires an
explicit warning confirmation whenever supersampling is enabled.

## Native Windows validation

Native Windows validation covered 3440x1440 output and several internal sizes.
The final release candidate was exercised at `RenderScale=1.15` (3956x1656),
with natural proportions, native camera FOV active and no fallback.
Supersampling itself remains experimental and disabled by default. The public
FOV configuration uses `1.200` as its tested recommendation under the corrected
single-owner camera model. Higher FOV values are accepted but untested.

Subsequent same-session near/far tests isolated a depth-aware crosshair defect
to the internal-width boundary. At 3956x1656 the reticle remained visible while
aiming at the same distant scene. At exactly 4096x1715 it was visible but began
to flicker, while at 4128x1728 it changed between visible and absent according
to aiming depth. It was also captured absent at 5160x2160. The cause was a pair
of signed 16-bit X conversions, with matching latent Y conversions. All four
now retain full 32-bit coordinates, and a native Windows gameplay retest at
5160x2160 internal confirmed the reticle working.
