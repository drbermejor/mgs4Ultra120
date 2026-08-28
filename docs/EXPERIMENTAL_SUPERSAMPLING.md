# Experimental supersampling

This feature is developed on the `experimental/supersampling` branch and is
published separately as the Windows-only alpha.6 prerelease. It is disabled by
default and is not part of the recommended alpha.5 release.

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

The initial `cc3c060` candidate was validated on native Windows at physical
3440x1440, `RenderScale=1.50` (5160x2160 internal), windowed presentation and
`FOVMultiplier=1.150`. Runtime verification reported a 3440x1440 client and
5160x2160 internal render state, 472 synchronized camera/frustum updates and no
late 16:9 fallback. The user confirmed the rendered result was correct. This is
an initial configuration test, not a full-playthrough or broad GPU-validation
claim; the feature therefore remains experimental and disabled by default.
