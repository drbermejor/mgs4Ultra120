# Windows installation

## Easy installation (recommended)

1. Download the **Windows ZIP** from Releases and extract it.
2. Exit the game.
3. Double-click **`MGS4Ultra120-Setup.cmd`**.
4. The setup normally finds the Steam game automatically. Click **Install / update**.
5. In the configurator that opens, choose any combination of fixes and click
   **Save settings**.
6. Start the game normally from Steam and select DirectX 12.

The same setup window can reopen the configurator or uninstall the patch. If
Windows displays a SmartScreen warning for the downloaded archive, choose
**More info > Run anyway** only after verifying that it came from this
repository's Releases page.

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
