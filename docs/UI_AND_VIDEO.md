# UI and pre-rendered video

The normal HUD remains the default. `v0.3.4-alpha.1` also includes a separate,
DX12-only `MGS4CenteredHUD16x9.asi` companion that can place conservatively
accepted HUD draws in one centered 16:9 safe area. The companion is experimental
and disabled by default.

The earlier prototype transformed every draw using a shared D3D12 vertex
shader, including full-screen fades and effects. The current classifier also
requires the known input layout, finite non-full-screen vertex bounds, a
conservative non-indexed draw and rejection of the large single-quad effect
class. Unknown, indexed and full-screen draws remain untouched. The original
viewport and scissor are restored after every accepted draw.

This improves gameplay HUD layout without claiming every menu is complete.
Save, pause, inventory, Codec, subtitle and prompt layouts can still contain a
misclassified or intentionally unusual draw. Set `Enabled=0` in
`mgs4_centered_hud_16x9.ini` if a problem appears; the companion then exits
before installing D3D12 hooks.

A 5120x2160 user reported a missing aiming crosshair and apparently zoomed FOV.
FOV 1.200 addresses the narrow framing through the validated native-camera
projection path. Native Windows supersampling
tests reproduced and isolated the separate reticle issue: it remains stable at
3956x1656 internal, flickers at exactly 4096 pixels wide, and can disappear
according to aiming depth at 4128x1728 and 5160x2160. The current release advises
an internal width below 4096. The cause is bounded but not yet patched.

Pre-rendered Bink 2 video is not cropped, stretched or replaced.
