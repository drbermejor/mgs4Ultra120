# Linux / Proton installation

The Linux package uses the same Ultimate ASI Loader and separate-plugin layout
as Windows. It provides ultrawide/Hor+, configurable FOV and resolution,
experimental supersampling, the controller-profile correction, optional direct
launch, optional corrected 30/60/120 FPS, optional real-time cinematic FOV and
an optional centered 16:9 HUD for DirectX 12.

The **experimental real-time cinematic FOV** control is disabled by default.
When enabled, use `inherit` to reuse the gameplay FOV or enter a different
cinematic multiplier, for example:

```ini
FOVMultiplier=1.200
ExperimentalCinematicFOV=1
CinematicFOVMultiplier=1.100
```

Pre-rendered videos are unaffected. Expanded framing can reveal characters,
objects or animation transitions earlier than the original shot intended.
The centered-HUD option is disabled by default and requires DX12. It is
currently a known-broken development preview: menus, subtitles, maps, text and
3D inventory previews can be misplaced, squashed, clipped or flicker. Keep it
disabled for normal play.

Validated environment: GE-Proton10-34, DirectX 12, KDE/Wayland, 3440x1440 and
the supported Steam executable. Other Proton versions may work but have not
passed the same release checks.

## Easy setup

1. Download and extract the Linux `.tar.gz` from the GitHub release.
2. Exit Steam completely, not only its game window.
3. Open a terminal in the extracted folder and run:

   ```bash
   ./MGS4Ultra120-Linux-Setup.sh
   ```

   For a custom Steam library, either select the folder containing `mgs4.exe`
   when prompted or pass it directly without editing any script:

   ```bash
   ./MGS4Ultra120-Linux-Setup.sh "/mnt/games/SteamLibrary/steamapps/common/METAL GEAR SOLID 4/MGS4"
   ```

   Running the top-level setup while the current directory itself contains
   `mgs4.exe` is also supported.

4. Setup searches the native and Flatpak Steam library records for app
   `2492670`. If it cannot identify one installation, select the folder that
   directly contains `mgs4.exe` in the graphical folder picker.
5. The installer downloads MGSFPSUnlock 0.1.0 directly from its official
   release, verifies it and applies the Proton compatibility byte locally.
6. The graphical configurator opens automatically when `zenity` is installed.
   Choose output resolution, FOV, optional cinematic FOV, optional centered
   HUD, supersampling, FPS, launcher mode and fullscreen mode, then save.
7. Start Steam and launch the game normally using DirectX 12.

The controller-profile workaround is opt-in. Alpha.5 resets it to disabled
during managed updates. If you previously needed it, reopen **MGS4 Ultra120
Configurator**, select **Enabled** for **Controller profile fix**, and save.
Manual users can set `ControllerProfileFixEnabled=1` under `[Input]` in
`mgs4_ultrawide.ini`. Keep it disabled for keyboard/mouse or hybrid controller
plus mouse/gyro input.

Setup also creates **MGS4 Ultra120 Configurator** on the desktop and in the
application menu. The installed tool lives in `~/.local/share/mgs4Ultra120`, so
the extracted download can be removed after setup.

The download contains no game files and no MGSFPSUnlock binary. Internet access
is required once when corrected FPS support is installed. To install only the
core patch and keep the game's normal FPS behavior:

```bash
MGS4_INSTALL_FPS_UNLOCK=0 ./MGS4Ultra120-Linux-Setup.sh
```

## Another Steam library

Easy Setup reads Steam's `libraryfolders.vdf` and the MGS4
`appmanifest_2492670.acf`, including the normal native and Flatpak Steam
locations. External drives and partitions therefore require no special
command when their library is registered in Steam.

If discovery fails or finds more than one valid copy, the Zenity folder picker
asks for the folder that directly contains `mgs4.exe`. Selecting the parent
`METAL GEAR SOLID 4` folder is also accepted. The successful path is stored in:

```text
~/.config/mgs4Ultra120/game-dir
```

If `XDG_CONFIG_HOME` is set, that directory is used instead of `~/.config`.

The desktop configurator and uninstaller reuse it automatically. To override
discovery for one command, pass the folder explicitly:

```bash
./MGS4Ultra120-Linux-Setup.sh "/path/to/METAL GEAR SOLID 4/MGS4"
```

Do not copy individual internal scripts into the game directory. Managed setup
replaces the desktop/application shortcut and includes the package version in
its title. If it still shows an older alpha, rerun the newest top-level setup
archive; an old title means an old configurator copy is being launched and
does not prove which ASI was installed.

## Configure later

Exit the game, then run:

```bash
./MGS4Ultra120-Linux-Configure.sh
```

After Easy Setup, the same configurator can be opened from its desktop shortcut
or the application menu without returning to the extracted archive.

The corrected FPS selector is active only when MGSFPSUnlock was installed.
Targets are 30, 60 and experimental 120 FPS. Supersampling keeps `Width` and
`Height` as the physical output and multiplies only the internal render size.
At 3440x1440, scale `1.15` produces 3956x1656. The old 4096-pixel reticle
overflow is fixed, but performance and VRAM requirements still rise with the
square of the selected scale.

## Fullscreen on KDE / Wayland

If the desktop panel remains visible, install Gamescope and select
**Gamescope fullscreen** in the configurator. For a 3440x1440 240 Hz display it
writes the reversible Steam option:

```text
WINEDLLOVERRIDES="winmm=n,b" gamescope -f --force-windows-fullscreen \
  -W 3440 -H 1440 -w 3440 -h 1440 -r 240 -- %command%
```

Native/no-Gamescope mode remains available. GameMode is not required or added.

## Uninstall

Exit Steam and run:

```bash
./MGS4Ultra120-Linux-Uninstall.sh
```

The uninstaller restores pre-existing loader, plugins, configuration, launcher
and Steam launch options from the installation backup.

See [corrected FPS on Proton](PROTON_FPS.md) for the verified local adaptation
and [experimental supersampling](EXPERIMENTAL_SUPERSAMPLING.md) for limits.
The previous `v0.3.3-alpha.1` release remains available as the complete legacy
fallback.
