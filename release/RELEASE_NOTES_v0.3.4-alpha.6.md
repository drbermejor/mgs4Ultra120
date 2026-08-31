# MGS4 Ultra120 v0.3.4-alpha.6

This release replaces the obsolete renderer-level centered-HUD experiment with
an earlier, native-layout implementation. Ultrawide rendering, FOV, corrected
FPS support, supersampling, the high-resolution reticle fix, controller option
and launcher bypass are otherwise unchanged.

## Native Centered HUD

`MGS4NativeCenteredHUD.asi` works on the game's native 1280x720 layout and a
small set of guarded surface/preview producers before the UI reaches D3D11 or
D3D12. It is a separate option and remains **experimental and disabled by
default**.

Validated on native Windows at 3440x1440:

- main menu and gameplay HUD centered without stretching;
- main pause/menu frames, labels and controls centered;
- subtitles and observed weapon/item/camouflage previews corrected;
- full ultrawide 3D world retained behind the centered interface.

Known limitations:

- map content is still horizontally compressed;
- Codec auxiliary scene/content is still horizontally compressed;
- original 16:9 title artwork may remain pillarboxed;
- Proton packaging is tested, but visual HUD validation under Proton is still
  pending;
- a complete playthrough may reveal further scene-specific exceptions.

Close the game and disable **Native Centered HUD** in the configurator to return
to the normal HUD without changing any other feature.

## Updating

Guided Windows and Linux setup removes the retired HUD companion only when an
installer ownership marker or the exact public alpha.5 hash matches. An unknown
or modified old ASI is preserved and setup stops with instructions instead of
deleting it.

An opt-in from the retired HUD experiment is not carried into the new module.
After updating, enable **Native Centered HUD** deliberately in the configurator
only if you accept the limitations above.

The controller-profile workaround remains opt-in. If you need it, enable
**Controller profile fix** in the configurator; leave it disabled for
keyboard/mouse or controller-plus-mouse/gyro input.

Use the Windows setup EXE/portable ZIP or Linux tarball from this release. The
manual ZIP remains available for a transparent copy-only installation.
