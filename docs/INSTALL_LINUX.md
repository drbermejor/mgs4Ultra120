# Linux / Proton installation

The Linux package uses the same Ultimate ASI Loader and separate-plugin layout
as Windows. It provides ultrawide/Hor+, configurable FOV and resolution,
experimental supersampling, the controller-profile correction, optional direct
launch and optional corrected 30/60/120 FPS.

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

4. The installer downloads MGSFPSUnlock 0.1.0 directly from its official
   release, verifies it and applies the Proton compatibility byte locally.
5. The graphical configurator opens automatically when `zenity` is installed.
   Choose output resolution, FOV, ultrawide, supersampling, FPS, launcher mode
   and fullscreen mode, then save.
6. Start Steam and launch the game normally using DirectX 12.

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

The default game directory is:

```text
~/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4
```

For another library, pass the folder that directly contains `mgs4.exe`:

```bash
MGS4_GAME_DIR="/path/to/METAL GEAR SOLID 4/MGS4" \
  ./MGS4Ultra120-Linux-Setup.sh
```

Use the same environment variable with the configure and uninstall launchers.

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
At 3440x1440, scale `1.15` produces 3956x1656 and remains below the known
4096-pixel reticle boundary.

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
