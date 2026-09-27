# MGS4 Ultra120 v0.3.4-alpha.8

> **MGSFPSUnlock and game version 1.4.1.** Easy Setup installs MGSFPSUnlock
> 0.1.0, which predates the 2026-09-11 game update (1.4.1). Players have
> reported cutscenes running too fast with desynchronized audio on 1.4.1 with
> older MGSFPSUnlock versions. Cipherxof's
> [MGSFPSUnlock 0.1.4](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.4)
> adds 1.4.1 support. Until Easy Setup is updated:
>
> - **Windows:** from `MGSFPSUnlock.zip`, copy only `scripts\MGSFPSUnlock.asi`
>   and `scripts\MGSFPSUnlock.ini` into `MGS4\scripts`, replacing the old
>   files. Do not copy `winmm.dll` or `wininet.dll`. In later MGS4 Ultra120
>   updates, leave **Install / update improved 120 FPS support** unchecked.
> - **Linux/Proton:** a manual update is not possible yet; if cutscenes run too
>   fast, select 60 FPS in the configurator.

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
