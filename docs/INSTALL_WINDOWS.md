# Windows installation

1. Download and extract the release package.
2. Exit the game and Steam.
3. Open PowerShell in the extracted directory and run:

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

It controls resolution, FOV, FPS and UI modes, validates the supported
executable hash, and warns before enabling 120 FPS. Command-line profiles also
remain available, for example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\configure.ps1 `
  -Profile stable
```

To uninstall, run `scripts\windows\uninstall.ps1` with the same `-GameDir`.
Pre-existing files are restored from the installer's private backup directory.
