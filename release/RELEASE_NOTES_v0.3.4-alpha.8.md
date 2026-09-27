# MGS4 Ultra120 v0.3.4-alpha.8

This release restores compatibility after the Steam update of MGS4 published
on 2026-09-11. Earlier Ultra120 releases detect the new `mgs4.exe` as an
unknown version and safely apply no changes. No feature, default or setting
behavior changed.

## Supported executable

| Field | Value |
|---|---|
| Executable | `MGS4/mgs4.exe` (Steam) |
| SHA-256 | `656ede900b03467e4ed05a00eca35f0ac306ce57a8f29a76a20e5a5410d02900` |
| PE timestamp | `0x6aa36b7c` (2026-09-11) |
| Internal version ID | `Pela_[MPA]_x64_BGFX_0.0.16_Release_ww_[Code]e9923cfe_[DataNew]4a9b66e6_2026_0911` |

The previous 2026-08-25 executable is no longer the supported profile. Steam
installs the new build automatically.

## What changed

- Both ASIs were ported to the new executable. Every code target and every
  Native Centered HUD function and caller route was relocated from its
  existing expected bytes and confirmed by its native call relationships.
- Two render-state data locations that moved in the new build were relocated:
  the controller connection mask and two resolution mirrors.
- The unknown-executable gate is unchanged: a future game update will again
  cause the mod to apply nothing until it is ported.

## Validation

On native Windows at 3440x1440 with the new executable:

- the full CTest suite passed on an MSVC Release build;
- the log reported every core hook installed: resolution getters and setter,
  native camera FOV, Hor+ projection and the four-site reticle fix;
- with Native Centered HUD enabled, every hook group installed: core layout,
  inventory previews, modal backgrounds, pause map, live Codec surface and
  Mission Briefing;
- the direct launcher wrapper started the game without the Unity launcher;
- a gameplay session behaved as expected, with no errors or rejected
  corrections logged.

That session was a smoke test, not a full scene-by-scene comparison. Report
any difference from alpha.7 with both log files.

## Updating

Use the Windows setup EXE/portable ZIP or Linux tarball from this release, or
the manual ZIP for a copy-only installation. Existing settings, including the
Native Centered HUD and controller-profile choices, are preserved.

## Known limitations

All alpha.7 limitations still apply:

- the live in-engine Codec 3D feed can still appear horizontally compressed;
- Mission Briefing control or ticker text can still overflow the centered
  canvas;
- Native Centered HUD remains experimental and disabled by default;
- visual validation under Proton is still pending.
