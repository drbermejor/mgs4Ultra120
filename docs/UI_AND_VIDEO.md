# UI and pre-rendered video

These are separate from the world projection fix.

## Current stable mode

`ConstrainUITo16x9=0` leaves the game's UI, menus and full-screen effects
untouched. Real-time engine cinematics use the corrected Hor+ projection.

## Centered 16:9 prototype

On D3D12, `ConstrainUITo16x9=1` recognizes one UI vertex shader and temporarily
uses a centered 16:9 viewport for matching draws. The game reuses that shader
for some full-screen fades and effects, so they can become narrowed, overlap or
leave uncovered regions. DirectX 11 does not have this prototype.

This mode is included for research and is not the planned final layout.

## Planned anchored ultrawide layout

The intended implementation preserves element scale but changes anchors:

- left HUD remains attached to the ultrawide left edge;
- right HUD moves to the ultrawide right edge;
- menus and central prompts remain centered;
- subtitles use a configurable central safe zone;
- true full-screen effects retain the entire output viewport.

At 3440x1440, a height-matched 16:9 canvas is 2560x1440 with 440 extra pixels
on each side. Correct layout therefore requires classifying left, center and
right anchors before final UI vertices are generated; applying a single
440-pixel translation to every draw is not correct.

## Pre-rendered video

The game ships Bink 2 (`.bk2`) assets and imports the decoder separately from
its renderer integration. This alpha does not crop, stretch or replace those
assets. A deterministic draw/compositor identification is required before the
patch can force a 16:9 pillarboxed rectangle without also affecting real-time
effects. Until that is verified, no heuristic video option is exposed.
