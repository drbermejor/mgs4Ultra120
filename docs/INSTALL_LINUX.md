# Linux / Proton installation

Linux is a separate release line. Use
`MGS4Ultra120-v0.3.1-alpha.5-linux.tar.gz` for the current core patch. Do not
run the Windows setup EXE or CMD under Proton.

The current Linux core provides ultrawide/Hor+ rendering, FOV adjustment, the
controller-profile correction and the optional direct-launch wrapper. It uses
the game's normal frame-rate behavior. The external MGSFPSUnlock integration
is Windows-only until its ASI-loader path has been validated under Proton.

The previously tested environment was Steam, GE-Proton10-34, the original
Unity launcher and DirectX 12. The alpha.5 Linux scripts fix the unconditional
`gamemoderun` dependency reported by users, but this refreshed package has not
yet received an end-to-end CachyOS retest.

## Install

1. Download the Linux tarball from the official release and extract it.
2. Exit Steam completely; do not merely close its window.
3. In a terminal inside the extracted folder, run:

   ```bash
   ./scripts/linux/install.sh
   ```

4. Force `GE-Proton10-34` for the game if it is installed.
5. Launch normally through Steam.

The installer adds `WINEDLLOVERRIDES="winmm=n,b"` without discarding existing
Steam launch options.

The default game directory is:

```text
~/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4
```

For another Steam library, pass the folder containing `mgs4.exe`:

```bash
MGS4_GAME_DIR="/path/to/METAL GEAR SOLID 4/MGS4" ./scripts/linux/install.sh
```

## Configure

Run:

```bash
./scripts/linux/configure.sh gui
```

The GUI controls ultrawide/FOV, the controller fix, the optional Unity-launcher
bypass and the Linux fullscreen command. The stable profile is available
without a GUI:

```bash
./scripts/linux/configure.sh stable
```

## KDE / Wayland panel remains visible

First confirm that this command works:

```bash
gamescope --help
```

Then select **Gamescope fullscreen** in the configurator, or use the following
Steam launch option with the monitor's native size and refresh rate:

```text
WINEDLLOVERRIDES="winmm=n,b" gamescope -f --force-windows-fullscreen \
  -W 3440 -H 1440 -w 3440 -h 1440 -r 240 -- %command%
```

GameMode is optional and is not inserted. The configurator leaves Steam launch
options unchanged if `gamescope` is missing. If the KDE panel remains visible,
try `Super+F` to toggle the Gamescope window or return to the native option:

```text
WINEDLLOVERRIDES="winmm=n,b" %command%
```

## Uninstall

Exit Steam and run:

```bash
./scripts/linux/uninstall.sh
```

Use the same `MGS4_GAME_DIR` override if one was used for installation. The
uninstaller restores files, launcher and Steam options only when their recorded
state still matches, so an external update is not overwritten blindly.
