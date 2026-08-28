# MGS4 Ultra120 v0.3.1-alpha.5 - Windows ASI release

Open-source ultrawide/FOV, 60/experimental 120 FPS and controller-profile patch
for the Steam PC version of METAL GEAR SOLID 4. The Windows patch uses the
Lyall-style ASI layout: Ultimate ASI Loader as `winmm.dll`, the project plugin
as `scripts/MGS4Ultra120.asi`, and an editable root INI.

## Windows downloads - choose one

### 1. Manual ZIP (most transparent)

Download `MGS4Ultra120-v0.3.1-alpha.5-windows-manual.zip`, extract it, close
the game, then copy everything inside the extracted folder into the `MGS4`
folder containing `mgs4.exe`. Keep the included `scripts` folder.

Final layout:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
```

Launch normally through Steam. This option runs no installer, CMD or
PowerShell script, creates no shortcuts and does not enable the optional Unity
launcher bypass. The INI can be inspected and edited in any text editor.

### 2. Portable ZIP

Download `MGS4Ultra120-v0.3.1-alpha.5-windows-portable.zip`, extract it and
double-click `MGS4Ultra120-Setup.cmd`. Choose **Install / update and
configure**, then **Save and close**. Keep the extracted folder to configure or
uninstall later.

### 3. Setup EXE

Run `MGS4Ultra120-v0.3.1-alpha.5-windows-setup.exe` for automatic Steam
detection, desktop/Start-menu shortcuts, configuration and reversible
uninstall. It installs the same patch as the ZIP routes.

The EXE is not digitally signed and may show an unknown-publisher or reputation
warning. It is entirely optional. The full source is public, so anyone can
inspect it, verify the adjacent SHA-256 file and build the installer
independently.

### 4. Complete ZIP

`MGS4Ultra120-v0.3.1-alpha.5-windows-complete.zip` contains both the portable
CMD route and a `Manual-Install` folder, plus all documentation, licences and
notices.

## Linux / Proton

The same release page includes the exact separately validated
`MGS4Ultra120-v0.3.1-alpha.2-linux.tar.gz`. Extract it and run
`./scripts/linux/install.sh` after closing Steam. Do not use the Windows files
under Proton; the Windows fixes do not imply new Linux validation.

## Refreshed alpha.5 fixes

- Accepts official launcher settings that omit redundant `WindowSizeW` and
  `WindowSizeH` fields while retaining `ResolutionWindowW/H`.
- Handles configurator save errors inside the GUI instead of displaying an
  unhandled-exception dialog.
- Reports Easy Setup success only after settings were actually saved.
- Keeps the previously validated ASI and pinned Ultimate ASI Loader binaries
  unchanged.

Verify every download with the adjacent `.sha256` file. This remains a public
alpha: 120 FPS and centered UI are experimental, and a full playthrough has
not yet been certified.
