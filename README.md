# MGS4 Ultra120

An open-source ultrawide, input-profile and experimental high-frame-rate patch
for the Steam PC port of *METAL GEAR SOLID 4*. Every feature is optional: a
user can enable ultrawide only, 120 FPS only, the controller fix only, any
combination of them, or all modules together.

> **Public alpha.** Back up your saves and read the limitations. Version
> `v0.3.1-alpha.3` is verified against one Steam executable. Other builds are
> blocked by default but can be attempted with an explicit unsafe override.
> The `v0.3.1-alpha.4` ASI migration is a separately published preview from a
> review branch. **Alpha.3 remains the recommended validated Windows release.**

## Modules

| Module | Default | What it changes |
|---|---:|---|
| Ultrawide and FOV | On | Native output size and Hor+ perspective projection |
| FPS override | On at 60 | 30/60/experimental 120 FPS, independently switchable |
| FPS hotkey | F10 | Toggles the active FPS override between 60 and 120 |
| Controller profile fix | On | Prevents a connected native gamepad from being spuriously reclassified as keyboard input |
| Windows presentation | Native window | Uses the primary monitor's physical size without an exclusive mode switch |
| Direct-launch wrapper | On | Skips the Unity launcher while Steam remains the parent launch path |
| Centered 16:9 UI | Off | Experimental D3D12 safe-area prototype |

The controller module does not emulate an Xbox pad, inject inputs or install a
system driver. It preserves the native controller family already detected by
the game and releases that state after all controllers disconnect.

## Rendering status

- 3440x1440 real-time 3D rendering is correctly proportioned and Hor+ instead
  of stretching a 16:9 image.
- The projection correction is in common engine code used by DirectX 11 and
  DirectX 12.
- Resolution and FOV are configurable for other displays.
- `FOVMultiplier=1.000` retains the original vertical FOV. Higher values widen
  both axes proportionally; `1.150` is an optional comfort setting.
- The stable UI mode leaves the original UI behavior untouched. The existing
  centered 16:9 D3D12 experiment can also affect full-screen effects and is
  therefore disabled by default.
- Pre-rendered Bink video is not cropped or expanded by this alpha. A dedicated
  forced-16:9 compositor path is still under investigation; real-time engine
  cinematics already use the corrected projection.

## Important limitations

- **120 FPS is not gameplay-safe yet.** It reaches 120 FPS, but a scripted
  intro has stalled while audio continued. Physics, QTE and script timing have
  not passed a full-playthrough test. Use the hotkey to return to 60 before
  cutscenes or scripted sequences.
- The centered UI mode is experimental, D3D12-only and can narrow or overlap
  full-screen effects. Leave it off for the stable rendering profile.
- Keyboard/mouse users should disable the controller-profile module. With a
  controller connected, the module deliberately rejects the game's erroneous
  switch to its keyboard profile.
- An unsupported executable override is available, but known data offsets
  cannot be made safe by a warning. Code-hook signatures are still checked;
  crashes remain possible and the override is off by default.
- A full playthrough and save compatibility are not yet certified.

## Platform downloads

Windows and Linux/Proton are maintained as separate validated release lines:

| Platform | Validated release | Download |
|---|---|---|
| Native Windows (recommended) | `v0.3.1-alpha.3` | Validated unsigned setup EXE or portable Windows ZIP |
| Native Windows ASI preview | `v0.3.1-alpha.4` | Review-branch prerelease; does not supersede alpha.3 |
| Linux / Proton | `v0.3.1-alpha.2` | Linux tarball from the previous release |

The alpha.3 release intentionally contains only Windows assets. Linux users
should keep the separately tested alpha.2 Linux package; do not substitute the
Windows setup/ZIP or infer Linux validation from the Windows fixes.

Download only from
[Releases](https://github.com/drbermejor/mgs4Ultra120/releases).

**Native Windows:** the recommended download is the setup EXE. It installs a setup
manager for the current user, creates Start-menu and desktop shortcuts, detects
Steam automatically and supports reversible removal. It is not digitally
signed; verify its published SHA-256 and do not disable SmartScreen or antivirus
globally. The portable ZIP and `MGS4Ultra120-Setup.cmd` route remain available.

The Windows stable profile detects the primary monitor's physical resolution
and requests a native-size window. In the tested 3440x1440 setup this occupied
the complete monitor without a title bar while avoiding an exclusive display
mode switch. Exclusive fullscreen remains available as an advanced option.

The alpha.4 preview follows the conventional Lyall-style ASI layout: pinned
Ultimate ASI Loader `v9.7.4` is the independent `winmm.dll` proxy and the patch
is `scripts/MGS4Ultra120.asi`. The INI remains beside `mgs4.exe`, so the GUI,
portable CMD and manual editor stay compatible. Setup migrates alpha.3 in
place, preserves its original backups, and reuses a compatible ASI loader from
another mod without claiming or removing it.

**Linux / Proton:** use only the Linux tarball and follow the separate Linux
guide. Its shell installer, Steam launch options and Gamescope path are not used
by the native-Windows installer.

- [Windows installation](docs/INSTALL_WINDOWS.md)
- [ASI migration preview and acceptance record](docs/ASI_MIGRATION.md)
- [Manual DLL and INI installation](docs/MANUAL_INI.md)
- [Windows troubleshooting](docs/TROUBLESHOOTING_WINDOWS.md)
- [Linux / Proton installation](docs/INSTALL_LINUX.md)
- [Configuration and independent combinations](docs/CONFIGURATION.md)
- [Direct-launch wrapper](docs/LAUNCHER_WRAPPER.md)
- [Controller profile fix](docs/CONTROLLER_FIX.md)
- [UI and pre-rendered video status](docs/UI_AND_VIDEO.md)

The patch never edits `mgs4.exe`. Alpha.3 loads through the project's
reversible `winmm.dll` proxy. The alpha.4 preview uses Ultimate ASI Loader as
`winmm.dll` and keeps project code in `scripts/MGS4Ultra120.asi`. The optional
direct-launch feature temporarily replaces `Launcher/launcher.exe`, retains a
private backup and restores it only
after verifying that the active file is our wrapper.

The alpha.3 wrapper uses the game's official `mgs4_param` bootstrap without
duplicating its tokens on the child command line. This removes the repeated
Steam custom-arguments loop seen with older wrappers on some clients. Cancel
and report any unexpected argument prompt instead of approving it repeatedly.

On the tested mixed-scaling, mixed-refresh HDR dual-monitor system, switching
focus reproduced a moving red band/flicker. A light occurrence was also seen in
a native-size window with all patch modules disabled. Auto HDR and windowed-game
optimizations did not eliminate it; Windows recorded display `WATCHDOG`
`LiveKernelEvent` reports. With **G-SYNC/VRR disabled**, ten focus transitions
were clean, including the final 3440x1440 test with every stable module enabled.
The ASI preview also passed 20 programmatically confirmed foreground changes
across the same monitors with zero new crash dumps or reliability/display
events. This is encouraging but does not establish the loader architecture as
a universal fix for a scan-out/VRR problem that desktop capture may miss.
The configurator warns on NVIDIA multi-monitor systems but never alters driver
settings. If a display remains affected after closing the game,
`Win+Ctrl+Shift+B` resets the Windows graphics pipeline. See the troubleshooting
guide before playing on mixed-DPI/HDR/VRR displays.

## How the world fix works

The port normally supplies a 16:9 perspective matrix even when its output
canvas is ultrawide. The patch recognizes those perspective matrices and
changes only their FOV scales:

```text
adjusted_m11 = original_m11 / FOVMultiplier
new_m00 = sign(m00) * abs(adjusted_m11) / target_aspect
```

At `1.000`, vertical FOV remains unchanged and horizontal FOV grows. The
resolution setters are hooked when state changes; there is no recurring memory
rewrite loop. See [Technical notes](docs/TECHNICAL.md) for safeguards and
known offsets.

Build instructions are in [Development](docs/DEVELOPMENT.md). Contributions
are welcome under the [MIT License](LICENSE). MinHook retains its BSD license
and Ultimate ASI Loader its MIT license; see
[third-party notices](THIRD_PARTY_NOTICES.md).

This is an unofficial community project. It contains no game files and is not
affiliated with or endorsed by KONAMI.
