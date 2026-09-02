# Native Centered HUD and video status

`v0.3.4-alpha.7` uses the optional `MGS4NativeCenteredHUD.asi` companion. It
operates on the game's
native 1280x720 layout converter and a small set of guarded native surface and
preview producers before the UI reaches D3D11 or D3D12.

The feature remains independent of resolution, FOV, FPS, supersampling,
controller and launcher behavior. It is disabled by default:

```ini
[NativeHUD]
Enabled=0
```

## What currently works

- Main gameplay HUD widgets are proportioned correctly inside a centered 16:9
  safe canvas while the 3D world remains full-width ultrawide.
- Main and pause-menu frames, labels and controls use the same native canvas.
- Subtitles and guarded movie surfaces are remapped from physical pixels.
- Observed weapon, item and camouflage preview routes are fitted uniformly so
  their 3D content is not squeezed horizontally.
- Exact verified modal backgrounds can remain output-covering while their
  controls remain centered.
- The pause-map plane and its live markers keep their intended aspect without
  changing the surrounding menu and legend.
- The verified live in-engine Codec feed uses a corrected auxiliary render
  width; static/prerendered Codec content keeps the normal centered path.
- The verified Mission Briefing root, two persistent owner rectangles and four
  child surfaces are fitted horizontally while full-output clears stay intact.
- The implementation is graphics-API independent and reads the live render
  dimensions, so it does not store a hard-coded 3440x1440 canvas.

## Known limitations

- The live Codec correction was validated on the identified in-engine feed;
  other Codec content types have not all been exercised.
- Mission Briefing normal composition was validated, but alternate ticker,
  help, fade and chapter-specific states are not exhaustively covered.
- The Drebin Shop preview route is guarded in code but has not been
  independently validated live.
- Original 16:9 title artwork may remain pillarboxed by design.
- The current visuals were validated on native Windows at 3440x1440. Package
  and configuration tests cover Linux, but visual validation under Proton is
  still pending.
- Coverage outside the verified native producers is not claimed. A complete
  playthrough may expose additional scene-specific exceptions.

These limitations are why the option remains experimental and disabled by
default. If a screen looks wrong, close the game and set `Enabled=0`; the HUD
companion then exits before installing hooks and the rest of MGS4 Ultra120 is
unchanged.

Do not combine this option with another HUD or layout modification. Installers
remove the package's retired HUD companion when ownership can be proven by its
marker or exact public release hash, and stop instead of deleting an unknown or
modified ASI.

## Independent switches

The remaining keys in `mgs4_native_centered_hud.ini` allow targeted diagnostic
fallbacks without changing the main patch:

```ini
CenterSubtitles=1
CenterMovies=1
CenterTVMovies=1
CenterInventoryPreviews=1
CorrectPauseMapAspect=1
CorrectCodecRealtimeAspect=1
CorrectMissionBriefingAspect=1
ExpandVerifiedFullscreenBackgrounds=1
```

They are read once at game startup. Restart the game after changing them.

Pre-rendered Bink framing outside these guarded native surfaces is otherwise
left unchanged by MGS4 Ultra120.
