# Windows installation

Use the official
[v0.3.4-alpha.1 release](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.4-alpha.1).
Every Windows package contains the same MGS4Ultra120 core. Improved 120 FPS is
an optional additive component supplied by
[cipherxof/MGSFPSUnlock](https://github.com/cipherxof/MGSFPSUnlock).

## Setup EXE

1. Download the setup EXE from the official release, close the game and run it.
2. Keep **Install / update improved 120 FPS support** checked for the recommended complete
   setup, or uncheck it for the offline core/normal-FPS route.
3. Confirm the detected folder directly contains `mgs4.exe`.
4. Click **Install / update and configure**.
5. Keep the recommended values and click **Save and close**.
6. Launch normally through Steam.

The 120 option downloads MGSFPSUnlock 0.1.0 from its official GitHub release
and rejects it unless both pinned SHA-256 checks pass. If download fails, the
core remains installed and the setup reports that 120 was skipped; retry later
without reinstalling the game.

Clearing the 120 FPS box skips installation or update; it does not delete a
copy that is already present. Use this project's uninstaller for a managed
copy, or remove a manually installed copy manually.

The EXE is unsigned and may trigger a browser or SmartScreen reputation notice.
It is optional: source, scripts and build instructions are public. Do not turn
off security tools globally. Use only the official release and avoid reuploads.

## Portable ZIP

1. Extract the portable ZIP completely.
2. Run `MGS4Ultra120-Setup.cmd` from the extracted folder.
3. Follow the same checked-120 or core-only choice described above.
4. Keep the extracted folder for later configuration or uninstall.

## Manual ZIP

This route runs no EXE, CMD or PowerShell:

1. Uninstall/remove old MGS4 Ultra120 builds. Keep exactly one ASI loader and
   one `scripts\MGS4Ultra120.asi`; do not mix release files.
2. Extract the manual ZIP.
3. Close the game.
4. Copy everything inside the extracted folder into the `MGS4` folder
   containing `mgs4.exe`.
5. Preserve the included `scripts` folder and launch through Steam.

Core layout:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\mgs4_centered_hud_16x9.ini
MGS4\scripts\MGS4Ultra120.asi
MGS4\scripts\MGS4CenteredHUD16x9.asi
```

For corrected 120 FPS, download the official
[MGSFPSUnlock 0.1.0 ZIP](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0)
and copy only `MGSFPSUnlock.asi` plus `MGSFPSUnlock.ini` from its `scripts`
folder into `MGS4\scripts`. Do not copy its alternative proxy DLLs because
`winmm.dll` already loads both plugins.

The copy-only route does not replace the Unity launcher. Most Steam clients can
approve its normal child launch, but if it returns to the launcher and repeats
the custom-arguments prompt described in
[Issue #2](https://github.com/drbermejor/mgs4Ultra120/issues/2), run the portable
setup once and save with **Skip Unity launcher** enabled. The current wrapper
reproduces the official launcher's `CreateProcessW` child command line. Steam
owns its temporary launch interception; the wrapper does not write that file.

## Complete ZIP

The complete ZIP contains the portable setup, a `Manual-Install` folder and all
documentation/notices. Choose one installation route; do not perform both.

## Defaults and safe updates

The recommended Windows profile uses the primary monitor's physical resolution,
native-size windowed presentation, the tested FOV 1.200 recommendation, controller-profile correction,
Unity-launcher bypass and —when selected—MGSFPSUnlock at 120.
Real-time cinematic FOV, centered 16:9 HUD and supersampling remain disabled by
default. Enable them individually in the configurator. Closing the game and
disabling the affected switch is sufficient to return to the reference
behavior; `v0.3.3-alpha.1` remains available as the complete legacy fallback.

The manual ZIP does not install the launcher bypass. Manual users select the
game language in the official Unity launcher; `Language=` affects only the
optional wrapper installed by guided setup.

Setup does not edit `mgs4.exe`. It backs up unknown same-name files, preserves
supported settings during updates and reuses a compatible x64 Ultimate ASI
Loader owned by another mod. A stale Steam library on a disconnected drive is
ignored instead of aborting detection.

Setup blocks a second Ultimate ASI Loader proxy or a renamed old MGS4 Ultra120
ASI because either can initialize the patch twice. It reports exact candidates
and never deletes third-party DLLs. Remove the old build or alternative loader
yourself after confirming which mod owns it, then run setup again.

## Command line

Core-only installation:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\install.ps1 `
  -GameDir "D:\SteamLibrary\steamapps\common\METAL GEAR SOLID 4\MGS4"
```

Core plus verified 120 component:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\install.ps1 `
  -GameDir "D:\SteamLibrary\steamapps\common\METAL GEAR SOLID 4\MGS4" `
  -IncludeImproved120
```

To uninstall, run `scripts\windows\uninstall.ps1` with the same `-GameDir`.
Owned files are removed and backed-up originals are restored only when ownership
checks still match.

See [configuration](CONFIGURATION.md),
[manual installation](MANUAL_INI.md) and
[Windows troubleshooting](TROUBLESHOOTING_WINDOWS.md).
