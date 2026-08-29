# MGS4 Ultra120 v0.3.3-alpha.2 — cinematic FOV preview

This is an **opt-in Windows and Linux/Proton preview**, not the recommended
release. For normal play, continue using
[`v0.3.3-alpha.1`](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1).

## What it tests

The preview can apply configurable FOV to real-time, in-engine cutscenes.

To use the **same multiplier for gameplay and cutscenes**:

```ini
FOVMultiplier=1.200
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=inherit
```

`inherit` makes the cinematic FOV use the normal `FOVMultiplier` value.

To use **separate gameplay and cinematic multipliers**:

```ini
FOVMultiplier=1.200
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=1.100
```

In this example, gameplay uses `1.200` and real-time cutscenes use `1.100`.
Values are not artificially capped, but `1.200` or lower is recommended for
initial 21:9 testing.

Pre-rendered videos are unaffected.

## Expected limitation: authored-scene pop-in

Expanded cinematic framing can expose characters, objects, geometry or
animation transitions before the original shot intended them to enter the
frame. They may appear suddenly near the newly visible edges.

This is scene pop-in or early visibility from showing more of the authored
shot. It is **not** the old projection/frustum culling regression discussed
during earlier FOV experiments.

## Windows installation

The Windows preview intentionally has no installer or complete package.

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

## Linux / Proton installation

1. Download and extract the Linux `.tar.gz` from this release.
2. Exit Steam completely.
3. Open a terminal in the extracted folder and run:

   ```bash
   ./MGS4Ultra120-Linux-Setup.sh
   ```

4. In the configurator, enable **Real-time cinematic FOV (experimental)**.
5. Enter `inherit` to reuse the gameplay FOV, or enter a separate cinematic
   multiplier.
6. Start Steam and launch the game normally using DirectX 12.

The package uses the same preview ASI as Windows through Proton. Its
install/configure/uninstall paths passed automated Ubuntu/WSL packaging tests;
real game behavior on Linux remains experimental and needs community
validation.

## Reverting

Set `ExperimentalCinematicFOV=0` to disable the preview path. To return fully
to the recommended build, close the game and reinstall the appropriate
`v0.3.3-alpha.1` Windows or Linux package.

## Reporting

Please use only
[tracking issue #5](https://github.com/drbermejor/mgs4Ultra120/issues/5).
Do not open a separate issue for each scene. Include the exact
scene/checkpoint, resolution, both FOV values, screenshot or video, and
`mgs4_ultrawide.log`.

Reports without enough reproduction information may not be investigated.
