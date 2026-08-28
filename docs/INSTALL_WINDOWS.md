# Windows installation

## Easy installation (recommended)

1. Download the **Windows ZIP** for the newest release and extract it. Do not
   use GitHub's automatically generated **Source code** archives.
2. Exit the game.
3. Double-click **`MGS4Ultra120-Setup.cmd`**.
4. The setup normally finds the Steam game automatically. Click **Install / update**.
5. In the configurator that opens, choose any combination of fixes and click
   **Save settings**.
6. Start the game normally from Steam and select DirectX 12.

The extracted folder name should contain the same version shown on the
Releases page. The `3440 is not a valid value` configurator error came from an
older Windows package and is fixed in `v0.3.1-alpha.2` and later.

Steam can display **Launch Game with custom arguments** after the original
Unity launcher starts the game. This is Steam's confirmation dialog, not an
MGS4 Ultra120 error. If the displayed command contains a complete quoted
`-launcherroot "...\\METAL GEAR SOLID 4\\launcher"` path, choose **Continue**.
Choosing **Cancel** cancels the launch. If **Continue** returns to the Unity
launcher and the same prompt repeats, close it, reopen the configurator and
enable **Skip the Unity launcher** before saving. This avoids Steam routing the
child request back through the launcher entry again.

`-ctrltype AUTO` in that dialog means the original Unity launcher is active.
The separate **Skip the Unity launcher** module is opt-in: enable it in the
configurator if desired. It is independent from the main `winmm.dll` patch,
which can load correctly with either launcher path.

The same setup window can reopen the configurator or uninstall the patch. If
Windows displays a SmartScreen warning for the downloaded archive, choose
**More info > Run anyway** only after verifying that it came from this
repository's Releases page.

## Verify that the patch loaded

After reaching `mgs4.exe` once, look for `mgs4_ultrawide.log` beside the game
executable (normally in the `MGS4` directory). A current successful load
contains a `Configuration:` line followed by installed-hook messages. If the
file is absent, the `winmm.dll` proxy was not loaded; if it contains an
`ERROR:` line, include that line in a bug report.

When the optional direct-launch module is enabled, its separate log is
`Launcher\\mgs4_direct_wrapper.log`. The existence of one log does not imply
that the other module is enabled.

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
path. The centered 16:9 UI path is experimental and disabled by default. The
installer copies `winmm.dll` and
`mgs4_ultrawide.ini` next to `mgs4.exe`; it never modifies the executable.

Open the graphical configurator while the game is closed:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1
```

It exposes independent switches for ultrawide/FOV, FPS, controller-profile
correction and the optional direct-launch wrapper. It validates the executable
hash and requires separate confirmations for 120 FPS and an unsupported-build
override. Command-line profiles also remain available, for example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -Profile stable
```

To uninstall, run `scripts\windows\uninstall.ps1` with the same `-GameDir`.
Pre-existing files are restored from the installer's private backup directory.
The launcher is restored only if its active hash still matches this package's
wrapper; a launcher changed by Steam or another tool is preserved.
