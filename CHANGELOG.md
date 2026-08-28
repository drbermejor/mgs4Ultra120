# Changelog

## v0.3.1-alpha.2

- Fixed the Windows configurator crashing while initializing render widths
  above the WinForms default maximum of 100.
- Confirmed the width and height controls accept values through 16384,
  including 5120x1440 displays.

## v0.3.1-alpha.1

- Added a Windows easy-setup window launched by double-clicking
  `MGS4Ultra120-Setup.cmd`.
- Added automatic discovery of the game across the default Steam folder and
  additional libraries listed in `libraryfolders.vdf`.
- Combined reversible install/update, configuration and uninstall entry points
  in the easy-setup window.
- Rewrote the Windows guide so the normal path requires no terminal commands
  or manual file copying.
- Kept the in-progress proportional/anchored UI rewrite out of release builds;
  only the previously documented legacy 16:9 UI experiment remains available.

## v0.3.0-alpha.1 - 2026-08-28

- Added an independent controller-profile fix that preserves the game's native
  connected pad family without emulation, polling or a system input layer.
- Added an optional Steam-path direct-launch wrapper with reversible,
  update-aware launcher backups.
- Added a configurable event-driven hotkey to switch the FPS module between 60
  and experimental 120 FPS.
- Expanded both graphical configurators so ultrawide, FPS, controller fix,
  launcher and UI choices can be combined independently.
- Added a warned `AllowUnsupportedExecutable` override while retaining hook
  signature checks and the safe blocked default.
- Documented Bink video behavior, the planned anchored UI design, controller
  behavior and all supported option combinations.

## v0.2.1-alpha.1 - 2026-08-27

- Added engine-level Hor+ correction at arbitrary aspect ratios for the common
  DirectX 11/DirectX 12 projection path.
- Corrected native render-surface dimensions before renderer initialization;
  the patch is event-driven and does not rewrite memory every few milliseconds.
- Added a proportion-preserving FOV multiplier with `1.000` as the original
  vertical-FOV profile.
- Added independent ultrawide/FOV and FPS override modules. Either can be
  disabled completely, including ready-made `fps-only-120` and
  `ultrawide-only` profiles.
- Added stable 60 FPS, optional 30 FPS and experimental 120 FPS values. The 120
  mode remains experimental because scripted-scene stalls have been observed.
- Added an optional centered 16:9 D3D12 UI path. It remains experimental
  because some fullscreen effects share the identified shader.
- Added reversible Windows and Linux installers, graphical configurators,
  executable validation, and reversible Linux native/Gamescope launch modes.
- Added fail-closed compatibility checks for unknown executable updates.
