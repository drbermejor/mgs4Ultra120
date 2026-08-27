# Compatibility

## Supported executable

This alpha intentionally fails closed on unknown builds.

| Field | Supported value |
|---|---|
| Store | Steam |
| Executable | `MGS4/mgs4.exe` |
| SHA-256 | `9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41` |
| File size | `32,682,568` bytes |
| PE timestamp | `0x6a8cfc47` (2026-08-25) |
| PE image size | `0x241be000` |
| Internal version ID | `Pela_[MPA]_x64_BGFX_0.0.3_Release_ww_[Code]a84606af_[DataNew]d06ab525_2026_0825` |

The primary tested setup is 3440x1440, DirectX 12, GE-Proton10-34 on Linux.
World projection and output-resolution hooks are common engine code and also
apply to DirectX 11; the selective UI safe area is D3D12-only in this alpha.

## Test status

- New-game cemetery/menu scenes: correct Hor+ proportions at 3440x1440.
- A current save loaded into 3440x1440 gameplay at 60 FPS with full-screen
  output and no repeated/corrupt right-side surface.
- The load-complete screen waits at `PULSE CUALQUIER BOTÓN` until explicit
  confirmation.
- FOV 1.000 and 1.150: visually compared from the same saved camera position;
  1.150 shows more vertical/horizontal scene content without changing object
  proportions.
- D3D12 UI: the centered 16:9 path reaches menus/HUD, but remains experimental
  because the same shader is used by some full-screen effects.
- 60 FPS: release default.
- 120 FPS: presentation rate reached, but a scripted intro stall with audio
  continuing was reproduced. Not certified for gameplay.
- A full playthrough is not yet certified; back up saves and treat the project
  as alpha software.
