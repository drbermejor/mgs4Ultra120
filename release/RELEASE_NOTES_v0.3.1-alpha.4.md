# MGS4 Ultra120 v0.3.1-alpha.4 - Native Windows ASI release

Alpha.4 introduced and validated the native-Windows ASI architecture. It was
subsequently promoted to `main` in alpha.5. Linux/Proton remains on its separate
prior release line.

## What changes

- The project patch is now `scripts/MGS4Ultra120.asi`.
- Pinned Ultimate ASI Loader `v9.7.4` supplies the independent x64
  `winmm.dll` forwarding/ASI-loading layer.
- The GUI, portable CMD and manual editing continue to share the root
  `mgs4_ultrawide.ini`.
- The legacy complete project WinMM proxy still builds and passes its export
  test so alpha.3 remains reproducible.

## Safe migration

- Setup recognizes alpha.3 ownership, preserves the original pre-install
  backup and migrates in place.
- Supported user INI values remain preserved.
- A same-name ASI is backed up and conditionally restored.
- A compatible Ultimate ASI Loader owned by another mod is reused and never
  removed by this package.
- Clean install, update, uninstall, displaced-file preservation and external
  loader coexistence passed automated smoke tests.

## Native test result

Alpha.4 launched through Steam on Windows 11 at physical 3440x1440/60 with
all stable hooks active and no custom-arguments prompt. Ultimate ASI Loader,
system WinMM and the project ASI were confirmed in the game process. Ten
cross-monitor cycles completed 20 confirmed foreground changes with the game
responsive, clean physical captures, no new crash dump and no new Windows
reliability/display record.

The user later completed manual focus-change testing without an apparent
recurrence and accepted the ASI build for the main Windows line. This remains a
public alpha rather than a full-playthrough certification.

## Security

The setup EXE is not digitally signed, so an unknown-publisher/reputation notice
is possible. The complete source and build instructions are public. Users can
compile it themselves, run the readable ZIP/CMD setup, or drag the contents of
`Manual-Install` into the folder containing `mgs4.exe`.

See `docs/ASI_MIGRATION.md` for architecture, exact upstream hashes, ownership
rules and the full acceptance record.
