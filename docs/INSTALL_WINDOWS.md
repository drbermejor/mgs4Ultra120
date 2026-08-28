# Windows installation

> **Recommended release:** use `v0.3.1-alpha.3` unless you intentionally want
> to test the `v0.3.1-alpha.4` ASI migration preview. Alpha.4 is published from
> a review branch and does not supersede the previously validated release.

## Easy installation (recommended)

1. Download the **Windows setup EXE** from Releases.
2. Verify its SHA-256 against the `.sha256` file published beside it.
3. Exit the game and run the EXE. It installs the setup manager for the current
   user and creates Start-menu and desktop shortcuts by default.
4. The setup normally finds the Steam game automatically. Click **Install / update**.
5. In the configurator that opens, choose any combination of fixes and click
   **Save settings**.
6. Start the game normally from Steam and select DirectX 12.

The stable Windows default is **Windowed at native size** with automatic
physical resolution detection. At 3440x1440 it fills the primary monitor like
a borderless window. On the tested mixed-refresh HDR dual-monitor system,
**Auto HDR had to be disabled** under **Settings > System > Display > Graphics**
initially appeared to eliminate a moving red sweep, but the fault later
returned with Auto HDR and windowed-game optimizations both off. The final
3440x1440 test passed repeated focus changes only with **G-SYNC/VRR disabled**.
On NVIDIA multi-monitor systems the configurator shows an explicit warning; it
never changes NVIDIA or Windows display settings. **Exclusive fullscreen**
remains available as an advanced option and requires a warning confirmation.
The configurator obtains the current physical display mode directly from
Windows, so desktop scaling such as 125% or 150% does not reduce the saved
render resolution.

The installer, setup manager and configurator use consistent English-only UI
text to avoid locale-dependent encoding problems. The normal path is one blue
**Install / update and configure** button, followed by **Save and close**;
users who do not want advanced changes can keep every recommended value
unchanged. The path field is validated before installation is enabled.

Use either shortcut to reopen setup/configuration. Windows Apps can uninstall
the setup manager; its uninstaller first restores the game files and original
Unity launcher safely.

The installer is **not digitally signed**, so Windows SmartScreen may identify
it as an unrecognized app or show **Unknown publisher**. Continue only if it
came from this repository's official Releases page and its SHA-256 matches.
Do not disable SmartScreen or antivirus protection globally. Cancel if the
source or hash is different.

Chrome may also block the EXE as dangerous or suspicious because it is a new,
unsigned executable with little reputation. This can be a false positive, but
never bypass it blindly: use only the official release, verify the adjacent
SHA-256 and keep antivirus enabled. If you do not trust the EXE, download the
Windows ZIP, verify it, extract the complete folder and double-click
`MGS4Ultra120-Setup.cmd`. The CMD opens the same inspectable PowerShell setup.

The ZIP remains available as the portable alternative: extract it and
double-click **`MGS4Ultra120-Setup.cmd`**. This CMD path is unchanged and opens
the same setup used by the EXE.

The default stable profile enables the reversible Unity-launcher bypass. The
alpha.3 wrapper places the required tokens in the game's official `mgs4_param`
bootstrap and does not repeat them on the child process command line. This
avoids the repeated **Start game with custom arguments** loop reported with
older wrappers. Cancel any unexpected argument prompt and report it rather than
repeatedly choosing Continue.

## Manual/advanced installation

Open PowerShell in the extracted directory and run:

   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\windows\install.ps1
   ```

The default location is the standard Steam library. For another library:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\install.ps1 `
  -GameDir "D:\SteamLibrary\steamapps\common\METAL GEAR SOLID 4\MGS4"
```

Launch normally through Steam and select DirectX 12 for the validated world
path. The centered 16:9 UI path is experimental and disabled by default.
Alpha.3 copies its combined `winmm.dll` patch and `mgs4_ultrawide.ini` next to
`mgs4.exe`. The alpha.4 preview instead installs Ultimate ASI Loader as
`winmm.dll`, the project plugin as `scripts/MGS4Ultra120.asi`, and keeps the
same root INI. Neither layout modifies the executable.

Alpha.4 recognizes an installed alpha.3 ownership marker and migrates it
without replacing the true pre-alpha backup. If another mod already supplied
a compatible x64 Ultimate ASI Loader, setup leaves that loader in place and
installs only the project ASI. Unknown `winmm.dll` and same-name ASI files are
backed up before replacement; uninstall removes only recorded project files.

Open the graphical configurator while the game is closed:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1
```

It exposes independent switches for ultrawide/FOV, FPS, controller-profile
correction, Windows presentation mode and the optional direct-launch wrapper.
It validates the executable hash and requires separate confirmations for 120
FPS, exclusive fullscreen and an unsupported-build override. Command-line
profiles also remain available, for example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -Profile stable
```

For an explicit presentation mode from the command line:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -Profile stable -WindowsDisplayMode Windowed
```

Before either mode starts, the configurator synchronizes the official
launcher's full/window size fields to the selected physical resolution. The
original values are recorded once and are conditionally restored on uninstall;
values changed later by the user are not overwritten.

To uninstall, run `scripts\windows\uninstall.ps1` with the same `-GameDir`.
Pre-existing files are restored from the installer's private backup directory.
The launcher is restored only if its active hash still matches this package's
wrapper; a launcher changed by Steam or another tool is preserved.

For a copy-only route and manual text editing of the same configuration used by
the GUI, see [Manual DLL and INI installation](MANUAL_INI.md).

For loader errors, Steam prompts and the observed multi-monitor flicker/reset
procedure, see [Windows troubleshooting](TROUBLESHOOTING_WINDOWS.md).
