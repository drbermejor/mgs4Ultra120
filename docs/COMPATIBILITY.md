# Compatibility

## Supported executable

This alpha blocks unknown builds by default. The configurator exposes an
explicit unsafe override for testing, but that does not make their RVAs
compatible and crashes remain possible.

| Field | Supported value |
|---|---|
| Store | Steam |
| Executable | `MGS4/mgs4.exe` |
| SHA-256 | `9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41` |
| File size | `32,682,568` bytes |
| PE timestamp | `0x6a8cfc47` (2026-08-25) |
| PE image size | `0x241be000` |
| Internal version ID | `Pela_[MPA]_x64_BGFX_0.0.3_Release_ww_[Code]a84606af_[DataNew]d06ab525_2026_0825` |

The native-Windows alpha.3 acceptance setup is an RTX 4090, a 3440x1440 240 Hz
primary monitor at 125% scaling, a 2560x1440 144 Hz secondary monitor at 150%,
DirectX 12, Auto HDR disabled and G-SYNC disabled. The separately maintained
Linux/Proton line was tested at 3440x1440
with GE-Proton10-34. World projection and output-resolution hooks are common
engine code and also apply to DirectX 11. The unverified UI safe-area prototype
is not included in current release builds.

The alpha.4 ASI release was migrated in place from the installed alpha.3 on
the same system. Steam loaded the pinned Ultimate ASI Loader, the real system
WinMM and `scripts/MGS4Ultra120.asi`; the stable log reported all hooks active
at physical 3440x1440/60. DWM measured the primary frame at exactly
3440x1440 physical pixels while the expected DPI-virtualized API view was
2752x1152. Ten cross-monitor focus cycles (20 confirmed foreground changes)
passed with the game responding, clean physical captures, no new dump and no
new reliability/display event. G-SYNC remained disabled during that automated
run. The user later completed manual focus-change testing without an apparent
recurrence; this still does not establish a universal result for every
driver/display setup.

## Test status

- New-game cemetery/menu scenes: correct Hor+ proportions at 3440x1440.
- A current save loaded into 3440x1440 gameplay at 60 FPS with full-screen
  output and no repeated/corrupt right-side surface.
- The load-complete screen waits at `PULSE CUALQUIER BOTÓN` until explicit
  confirmation.
- FOV 1.000 and 1.150: visually compared from the same saved camera position;
  1.150 shows more vertical/horizontal scene content without changing object
  proportions.
- Synchronized FOV/culling build at 3440x1440 and FOV 1.150: hundreds of camera
  projections and frustums rebuilt one-for-one without a late fallback or
  startup crash; aiming crosshair confirmed visible by the user.
- Tight in-engine close-up from the same saved sequence at 3440x1440 and FOV
  1.150: reproduced the former momentary narrowing, captured its projection
  scale crossing the generic safety ceiling, then passed twice after the
  camera-specific continuity fix (first with a bounded diagnostic candidate and
  again with the final structural validation).
- UI: original game behavior; the previous centered 16:9 prototype was removed.
- Experimental branch only: internal supersampling at 3440x1440 output,
  1.50x/5160x2160 render resolution, windowed presentation and FOV 1.150 passed
  the first native-Windows runtime and visual check. Follow-up aiming tests found
  the reticle stable at 3956x1656, flickering at exactly 4096 pixels wide and
  depth-conditionally absent at 4128x1728 and 5160x2160. Alpha.6 warns users to
  keep internal width below 4096; supersampling remains disabled by default and
  is not part of the alpha.5 release.
- 60 FPS/core: previously validated Windows path remains available without the
  optional high-FPS component.
- Corrected 120 FPS: supplied by optional `cipherxof/MGSFPSUnlock` 0.1.0 and
  verified here for package installation/coexistence. A full playthrough is not
  yet certified by this project.
- Controller profile fix: native controller profile, full disconnect and
  reconnection were exercised under Proton without a virtual controller layer.
- Native Windows: clean install through the unsigned EXE, Steam launch with no
  visible child arguments, all stable modules at physical 3440x1440/60 FPS,
  repeated dual-monitor focus changes and setup-manager update/uninstall smoke
  paths passed. The test is not a full playthrough.
- Multi-monitor focus: ten transitions passed with G-SYNC disabled, including
  five at physical 3440x1440 with all stable modules active. Earlier runs with
  G-SYNC active produced a red sweep and Windows display WATCHDOG reports even
  after Auto HDR/windowed optimizations were disabled.
- Direct-launch wrapper: exercised through Steam on native Windows and
  Linux/Proton. The native child command line remained clean while launch data
  was consumed from the official bootstrap file.
- A full playthrough is not yet certified; back up saves and treat the project
  as alpha software.
