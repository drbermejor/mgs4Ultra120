# MGS4 Ultra120 v0.3.3-alpha.2 — cinematic FOV preview

This is an **opt-in Windows preview**, not the recommended release. For normal
play, continue using
[`v0.3.3-alpha.1`](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1).

## What it tests

The preview can apply configurable FOV to real-time, in-engine cutscenes while
keeping gameplay and cinematic multipliers independent:

```ini
FOVMultiplier=1.200
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=inherit
```

`inherit` uses the gameplay value. Advanced testers can enter a separate
cinematic multiplier such as `1.100`. Values are not artificially capped, but
`1.200` or lower is recommended for initial 21:9 testing.

Pre-rendered videos are unaffected.

## Expected limitation: authored-scene pop-in

Expanded cinematic framing can expose characters, objects, geometry or
animation transitions before the original shot intended them to enter the
frame. They may appear suddenly near the newly visible edges.

This is scene pop-in or early visibility from showing more of the authored
shot. It is **not** the old projection/frustum culling regression discussed
during earlier FOV experiments.

## Installation

This preview intentionally has no installer or complete package.

1. Install and verify `v0.3.3-alpha.1` first.
2. Close the game.
3. Ensure there is only one project ASI at
   `MGS4\scripts\MGS4Ultra120.asi`.
4. Extract the preview ZIP and replace that one ASI with the preview file.
5. Add `ExperimentalCinematicFOV=1` and
   `CinematicFOVMultiplier=inherit` under `[Ultrawide]` in
   `MGS4\mgs4_ultrawide.ini`.
6. Launch normally through Steam.

Do not copy the preview over alpha.5, alpha.6 or a mixed installation. Do not
keep the old ASI under another `.asi` filename.

## Reverting

Set `ExperimentalCinematicFOV=0` to disable the preview path. To return fully
to the recommended build, close the game and restore
`v0.3.3-alpha.1`'s `MGS4Ultra120.asi` or reinstall its Manual ZIP.

## Reporting

Please use only
[tracking issue #5](https://github.com/drbermejor/mgs4Ultra120/issues/5).
Do not open a separate issue for each scene. Include the exact
scene/checkpoint, resolution, both FOV values, screenshot or video, and
`mgs4_ultrawide.log`.

Reports without enough reproduction information may not be investigated.
