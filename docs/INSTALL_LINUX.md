# Linux / Proton installation

The tested path is Steam, GE-Proton10-34, the original Unity launcher, and the
DirectX 12 renderer. Keeping the launcher preserves the normal Steam/Cloud
workflow.

1. Download and extract the release package.
2. Exit Steam completely. Do not merely close its window.
3. From the extracted package, run:

   ```bash
   ./scripts/linux/install.sh
   ```

4. In Steam, force `GE-Proton10-34` for the game if it is installed.
5. Launch through Steam. The installer adds
   `WINEDLLOVERRIDES="winmm=n,b"` without discarding existing launch options.

If a desktop panel remains visible under KDE/Wayland, wrap the existing Steam
launch command in Gamescope, substituting the monitor's native mode:

```text
WINEDLLOVERRIDES="winmm=n,b" gamescope -f --force-windows-fullscreen \
  -W 3440 -H 1440 -w 3440 -h 1440 -r 240 -- gamemoderun %command%
```

Run `./scripts/linux/configure.sh gui` for the graphical configurator. It
controls independent ultrawide/FOV, FPS, controller-profile, launcher and UI
modules. It can also switch Steam
between native and Gamescope fullscreen commands without losing the launch
options backed up by the installer. Steam must be fully closed for that last
operation. The stable profile keeps FOV 1.000, original UI and 60 FPS;
centered UI and 120 FPS are separate experiments.

The default game directory is:

```text
~/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4
```

For another library, pass the directory containing `mgs4.exe`:

```bash
MGS4_GAME_DIR="/path/to/METAL GEAR SOLID 4/MGS4" ./scripts/linux/install.sh
```

To remove the patch, exit Steam and run `./scripts/linux/uninstall.sh` with the
same `MGS4_GAME_DIR` if one was used. The uninstaller restores files and Steam
launch options only when they still match the installer's recorded state.
The optional Unity launcher replacement receives the same hash-checked restore
protection, so a launcher updated by Steam is never overwritten blindly.
