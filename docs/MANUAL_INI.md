# Manual copy installation and INI editing

The manual ZIP contains the three files owned by MGS4Ultra120:

```text
winmm.dll
mgs4_ultrawide.ini
scripts\MGS4Ultra120.asi
```

Close the game and copy everything inside the extracted manual folder into the
folder that directly contains `mgs4.exe`. Keep the `scripts` folder.

## Add corrected 120 FPS manually

1. Download `MGSFPSUnlock.zip` from the official
   [MGSFPSUnlock 0.1.0 release](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0).
2. Open the ZIP and enter `MGSFPSUnlock\scripts`.
3. Copy only `MGSFPSUnlock.asi` and `MGSFPSUnlock.ini` to `MGS4\scripts`.
4. Do not copy the external package's `d3d11.dll`, `winhttp.dll` or
   `wininet.dll`; `winmm.dll` already loads both ASIs.
5. Open `MGS4\scripts\MGSFPSUnlock.ini` and confirm:

   ```ini
   [Settings]
   TargetFrameRate = 120
   ```

6. Launch normally through Steam.

Final layout:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\scripts\MGS4Ultra120.asi
MGS4\scripts\MGSFPSUnlock.asi
MGS4\scripts\MGSFPSUnlock.ini
```

Back up every same-name file before replacing it. If another mod already owns a
compatible x64 Ultimate ASI Loader, keep that loader and copy only the two ASIs
and two INIs. Use one proxy loader, not several.

The manual route creates no shortcuts and does not install the Unity-launcher
bypass. The MGS4Ultra120 GUI and manual edits remain compatible because they use
the same INIs.

To remove a purely manual installation, close the game and delete only files
you personally copied. Never delete a pre-existing loader belonging to another
mod; restore your backups instead.
