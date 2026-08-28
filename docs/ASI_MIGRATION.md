# ASI migration and acceptance record

## Status

The ASI layout introduced and validated in `v0.3.1-alpha.4` is the current
native-Windows architecture. `v0.3.1-alpha.5` promotes it to `main` and adds a
ready-to-drag manual package without changing the tested plugin or loader.
Linux and Proton remain on their separate prior release line.

## Layout

```text
MGS4/
  mgs4.exe
  winmm.dll                     Ultimate ASI Loader v9.7.4 x64
  mgs4_ultrawide.ini            shared GUI/manual configuration
  scripts/
    MGS4Ultra120.asi            project patch
```

The game already imports WinMM, so Windows loads the local proxy at startup.
Ultimate ASI Loader forwards WinMM to the system library and loads the project
plugin. This removes the WinMM export table from project patch code while
retaining the early, renderer-independent load point validated by alpha.3.

## Pinned upstream component

The loader is downloaded only by the developer build script; release ZIP/EXE
files contain the verified binary. Source control contains no upstream binary.

- Ultimate ASI Loader: `v9.7.4`
- Asset: `Ultimate-ASI-Loader-NoPDB_x64.zip`
- Archive SHA-256: `e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1`
- Extracted DLL SHA-256: `031a3e5576d91dce1e438d36b9a3d462c7334ab4791990a8ff1e3ddc0e132daf`
- License: MIT

The upstream binary has a timestamped self-signed `FusionFix` certificate, not
a certificate rooted in the Windows trusted publisher store. Users must still
expect reputation warnings and verify the release checksum.

## Migration and ownership rules

- An installed alpha.3 proxy recognized by its ownership marker or published
  hash is replaced by the pinned loader without replacing the true pre-alpha
  backup.
- `mgs4_ultrawide.ini` remains in the game root and supported settings are
  merged into the new template.
- A pre-existing `scripts/MGS4Ultra120.asi` is backed up. Updates recognize the
  exact installed-plugin hash; uninstall restores the original only when the
  active file still belongs to this package.
- A compatible x64 Ultimate ASI Loader supplied by another mod is reused. Its
  hash is recorded as external and uninstall leaves it in place even if the
  other mod updates it later.
- An unknown `winmm.dll` is backed up before the pinned loader replaces it.
- Launcher-wrapper ownership and official display-setting restoration retain
  the alpha.3 conditional-hash rules.

## Native acceptance result

Alpha.4 was installed over the final alpha.3 on Windows 11 with an RTX
4090, a 3440x1440 240 Hz primary at 125% scaling and a 2560x1440 144 Hz
secondary at 150% scaling. G-SYNC and Auto HDR were disabled.

- Steam launch completed without a custom-arguments prompt.
- Process modules contained local Ultimate ASI Loader, system WinMM and
  `scripts/MGS4Ultra120.asi`.
- The stable plugin log confirmed 3440x1440/60, Hor+, resolution, controller
  and hotkey hooks, with 408 projection matrices corrected during the sample.
- DWM measured the game frame at exactly 3440x1440 physical pixels. The
  2752x1152 value seen through a DPI-virtualized caller is expected at 125%.
- Ten cross-monitor cycles produced 20 confirmed foreground transitions while
  the game remained responsive.
- Physical desktop captures were clean, with no new crash dump and no new
  Windows reliability/display record.
- The game and wrapper closed normally through `WM_CLOSE`.

The user subsequently completed the manual focus-change validation without an
apparent recurrence and accepted the ASI architecture for the main Windows
line. This is not
a full playthrough, and it is not a universal claim for every NVIDIA driver or
mixed-monitor configuration.

## Acceptance status

The migration, clean installation, Steam launch, automated loader/plugin tests,
backup ownership rules, uninstall and manual focus-change validation have all
passed. DX11/DX12 coverage and a full playthrough remain useful independent
follow-up testing for this public alpha.
