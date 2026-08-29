MGS4 Ultra120 - experimental cinematic FOV preview
===================================================

This is not the recommended release. Install and verify v0.3.3-alpha.1 first:
https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1

INSTALL

1. Close the game.
2. Confirm that only one MGS4Ultra120 ASI exists.
3. Copy scripts\MGS4Ultra120.asi from this ZIP to the game's MGS4\scripts
   folder and replace the existing file.
4. Open MGS4\mgs4_ultrawide.ini and add these lines under [Ultrawide]:

   ExperimentalCinematicFOV=1
   CinematicFOVMultiplier=inherit

5. Launch normally through Steam.

"inherit" uses the normal FOVMultiplier. Advanced testers may use a separate
finite value of 0.500 or greater. Start at 1.200 or lower for 21:9.

EXPECTED LIMITATION

A wider in-engine cutscene may reveal characters, objects, geometry or
animation transitions before the original shot intended them to enter the
frame. This scene pop-in/early visibility is distinct from the old
projection/frustum culling regression. Pre-rendered video is unaffected.

REVERT

Set ExperimentalCinematicFOV=0, or restore the v0.3.3-alpha.1 ASI for a full
rollback. Never keep two project ASIs under different filenames.

Report results only in the centralized tracking issue:
https://github.com/drbermejor/mgs4Ultra120/issues/5

Include the scene, resolution, both FOV values, screenshot/video and
mgs4_ultrawide.log.
