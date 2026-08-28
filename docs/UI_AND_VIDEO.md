# UI and pre-rendered video

The released Windows ASI does not contain the unfinished UI safe-area hooks.
HUD, menus, subtitles and full-screen effects retain the game's original draw
behavior while real-time 3D projection receives the Hor+ correction.

The previous prototype identified a shared D3D12 vertex shader, but that shader
was also used by full-screen fades and effects. Constraining every matching draw
could narrow effects without producing correct left/right HUD anchoring. The
option and configurator controls were therefore removed instead of presenting
an unverified UI mode as a fix.

A 5120x2160 user has reported a missing aiming crosshair and apparently zoomed
FOV with MGS4Ultra120. That report concerns the ultrawide/projection path, not
MGSFPSUnlock, and remains open. 3440x1440 is the currently validated ultrawide
target.

Pre-rendered Bink 2 video is not cropped, stretched or replaced. A future
native-looking HUD implementation requires identifying individual draw classes
and applying left, center and right anchors independently; one global viewport
translation is not sufficient.
