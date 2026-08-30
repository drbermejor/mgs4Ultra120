# UI and pre-rendered video

The normal HUD remains the default. `v0.3.4-alpha.4` includes a separate,
DX12-only `MGS4CenteredHUD16x9.asi` companion that can place conservatively
accepted HUD draws in one centered 16:9 safe area. The companion is experimental
and disabled by default.

The earlier prototype corrected individual backend draws. The game can upload
the same UI through two emitter paths, so that late strategy could alternate
between corrected and original coordinates. Alpha.4 instead copies each whole
common-emitter record batch, redirects only its 2D affine transforms to bounded
D3D12 upload memory, calls the original emitter and restores the game's pointer.
This covers indexed text and scaled or animated 2D sub-canvases while rejecting
positively identified fullscreen quads.

An early build could briefly alternate between the centered and original HUD
after running for a while, most visibly in the dynamic weapon/item panels. Its
D3D12 resource tracker admitted textures into a fixed table and retained only
the latest copied slice of a dynamic UI buffer. The corrected tracker ignores
textures, recycles stale buffer entries safely and preserves writes at their
real offsets after a resource is identified as UI. A repeated 3440x1440 Windows
test remained stable beyond the previous failure interval. The same companion
binary runs under Proton, but Linux still requires a repeat visual test.

Weapon and item models are not 2D emitter records. Their auxiliary 3D viewport
is uniformly scaled and moved into the same safe area, with the paired scissor
changed once. This preserves model proportions and leaves the preview camera,
FOV and projection untouched. The broader output-render-target classifier is
still private; the public build uses the layout validated with the AK-102 and
knife at 3440x1440.

This improves gameplay HUD layout without claiming every menu is complete.
Save, pause, inventory, Codec, subtitle and prompt layouts can still contain a
misclassified or intentionally unusual draw. Set `Enabled=0` in
`mgs4_centered_hud_16x9.ini` if a problem appears; the companion then exits
before installing D3D12 hooks.

A 5120x2160 user reported a missing aiming crosshair and apparently zoomed FOV.
FOV 1.200 addresses the narrow framing through the validated native-camera
projection path. Native Windows supersampling tests reproduced and isolated the
separate reticle issue: signed 16-bit coordinate conversions overflowed at the
screen centre when an internal axis reached 4096 pixels. The X and Y routes now
retain their full 32-bit values; gameplay validation confirmed the reticle
working at 3440x1440 output with a 5160x2160 internal render.

Pre-rendered Bink 2 video is not cropped, stretched or replaced.
