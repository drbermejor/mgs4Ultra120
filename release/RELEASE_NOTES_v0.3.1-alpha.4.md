# MGS4 Ultra120 v0.3.1-alpha.4 - ASI migration preview

This is a Windows-only preview published from an open pull request.

**It does not supersede v0.3.1-alpha.3. Alpha.3 remains the recommended and
previously validated native-Windows release.** Linux/Proton remains on its
separate prior release line.

## What changes

- The project patch is now `scripts/MGS4Ultra120.asi`.
- Pinned Ultimate ASI Loader `v9.7.4` supplies the independent x64
  `winmm.dll` forwarding/ASI-loading layer.
- The GUI, portable CMD and manual editing continue to share the root
  `mgs4_ultrawide.ini`.
- The legacy complete project WinMM proxy still builds and passes its export
  test so alpha.3 stays reproducible during review.

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

The preview launched through Steam on Windows 11 at physical 3440x1440/60 with
all stable hooks active and no custom-arguments prompt. Ultimate ASI Loader,
system WinMM and the project ASI were confirmed in the game process. Ten
cross-monitor cycles completed 20 confirmed foreground changes with the game
responsive, clean physical captures, no new crash dump and no new Windows
reliability/display record.

G-SYNC remained disabled. This result does not prove that ASI resolves the
earlier NVIDIA mixed-refresh red sweep/flicker, and the preview is not a full
playthrough certification.

## Security

The setup EXE and project ASI are unsigned. The upstream loader carries a
timestamped self-signed certificate that is not rooted in the Windows trusted
publisher store. Chrome, SmartScreen or antivirus may show reputation-based
warnings. Download only from this release, verify the adjacent SHA-256 files
and never disable security protection globally. The portable ZIP/CMD route is
available for users who do not trust the setup EXE.

See `docs/ASI_MIGRATION.md` for architecture, exact upstream hashes, ownership
rules and the full acceptance record.
