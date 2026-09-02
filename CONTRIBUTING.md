# Contributing

Thank you for helping improve MGS4 Ultra 120.

## Before opening a change

1. Do not upload game executables, launcher files, archives, extracted assets,
   keys, full memory dumps, or RenderDoc captures containing game resources.
2. State the game version, store, executable SHA-256, renderer, OS/Proton
   version, resolution, and GPU in bug reports.
3. Reproduce visual changes in at least two real-time 3D scenes. Menus alone
   are not sufficient to validate projection.
4. Keep 3D projection, UI layout, pre-rendered video, input, and frame timing
   as separate problem areas when reporting or implementing changes.
5. New executable support must fail closed and validate a signature or a
   complete set of PE properties before writing memory.

## Building

Follow [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Keep the build reproducible,
format new code consistently with the surrounding source, and explain any new
RVA or signature in `docs/TECHNICAL.md`.

## AI-assisted tools

AI-assisted tools may be used for research, drafting and code review. They do
not replace maintainer responsibility. Every contribution, regardless of how
it was produced, must be reviewed for correctness and licensing, pass the
project's automated checks, and complete the applicable in-game validation.
Do not commit chat transcripts, prompts or internal working notes.

## Testing checklist

- Install and uninstall leave pre-existing proxy DLLs and INI files intact.
- 16:9 perspective matrices become the requested aspect ratio.
- Non-perspective and already-correct matrices remain unchanged.
- A DX11 and a DX12 launch both reach a real-time 3D scene.
- At 120 FPS, test gameplay, physics interactions, scripts, QTEs, and audio;
  report exact locations for any timing fault.

By contributing code, you agree that it may be distributed under this
repository's MIT License. Retain third-party copyright notices when required.
