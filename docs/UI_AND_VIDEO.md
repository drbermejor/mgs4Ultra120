# UI and pre-rendered video

The released Windows ASI does not contain the unfinished UI safe-area hooks.
HUD, menus, subtitles and full-screen effects retain the game's original draw
behavior while real-time 3D projection receives the Hor+ correction.

The previous prototype identified a shared D3D12 vertex shader, but that shader
was also used by full-screen fades and effects. Constraining every matching draw
could narrow effects without producing correct left/right HUD anchoring. The
option and configurator controls were therefore removed instead of presenting
an unverified UI mode as a fix.

A 5120x2160 user reported a missing aiming crosshair and apparently zoomed FOV.
FOV 1.150 addresses the narrow framing through the validated renderer-level
projection path. Native Windows supersampling
tests reproduced and isolated the separate reticle issue: it remains stable at
3956x1656 internal, flickers at exactly 4096 pixels wide, and can disappear
according to aiming depth at 4128x1728 and 5160x2160. Alpha.6 therefore advises
an internal width below 4096. The cause is bounded but not yet patched.

Pre-rendered Bink 2 video is not cropped, stretched or replaced. A future
native-looking HUD implementation requires identifying individual draw classes
and applying left, center and right anchors independently; one global viewport
translation is not sufficient.
