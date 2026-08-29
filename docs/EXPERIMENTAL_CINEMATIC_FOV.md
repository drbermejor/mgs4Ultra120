# Experimental cinematic FOV preview

The recommended release remains
[`v0.3.3-alpha.1`](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1).
This preview is intended only for users who want to help test expanded FOV in
real-time, in-engine cutscenes.

It does not affect pre-rendered video.

## Settings

Add these values under `[Ultrawide]` in `mgs4_ultrawide.ini`:

```ini
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=inherit
```

`inherit` uses the normal gameplay `FOVMultiplier`. To test a different
cinematic value, replace it with a finite value of at least `0.500`:

```ini
FOVMultiplier=1.200
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=1.100
```

The preview recommendation for 21:9 is `1.200` or lower. This is guidance, not
an enforced ceiling.

## Expected limitation

Increasing cinematic FOV shows portions of authored shots that the original
camera never exposed. Characters, objects, geometry or animation transitions
may therefore appear suddenly near the expanded edges, or become visible
before the director intended them to enter the frame.

This is scene pop-in or early visibility caused by exposing more of the shot.
It is distinct from the projection/frustum culling regression investigated in
earlier builds.

## Reporting

Use only the tracking issue linked from the preview release. Include:

- exact chapter, scene and checkpoint;
- resolution and aspect ratio;
- gameplay and cinematic FOV values;
- whether the problem disappears with `ExperimentalCinematicFOV=0`;
- a screenshot or short video;
- `mgs4_ultrawide.log`.

Reports without enough reproduction information may not be investigated. Do
not open separate issues for this preview.
