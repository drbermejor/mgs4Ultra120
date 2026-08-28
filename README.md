# MGS4 Ultra120

An open-source ultrawide, input-profile and experimental high-frame-rate patch
for the Steam PC port of *METAL GEAR SOLID 4*. Every feature is optional: a
user can enable ultrawide only, 120 FPS only, the controller fix only, any
combination of them, or all modules together.

> **Public alpha.** Back up your saves and read the limitations. Version
> `v0.3.1-alpha.1` is verified against one Steam executable. Other builds are
> blocked by default but can be attempted with an explicit unsafe override.

## Modules

| Module | Default | What it changes |
|---|---:|---|
| Ultrawide and FOV | On | Native output size and Hor+ perspective projection |
| FPS override | On at 60 | 30/60/experimental 120 FPS, independently switchable |
| FPS hotkey | F10 | Toggles the active FPS override between 60 and 120 |
| Controller profile fix | On | Prevents a connected native gamepad from being spuriously reclassified as keyboard input |
| Direct-launch wrapper | Off | Skips the Unity launcher while Steam remains the parent launch path |
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

## Easy installation

Download the Windows ZIP or Linux tarball from
[Releases](https://github.com/drbermejor/mgs4Ultra120/releases).

**Windows:** extract the ZIP, exit the game, and double-click
`MGS4Ultra120-Setup.cmd`. It detects Steam automatically, installs reversibly,
and opens the graphical configurator. No PowerShell commands or manual file
copying are required for the normal path.

- [Windows installation](docs/INSTALL_WINDOWS.md)
- [Linux / Proton installation](docs/INSTALL_LINUX.md)
- [Configuration and independent combinations](docs/CONFIGURATION.md)
- [Direct-launch wrapper](docs/LAUNCHER_WRAPPER.md)
- [Controller profile fix](docs/CONTROLLER_FIX.md)
- [UI and pre-rendered video status](docs/UI_AND_VIDEO.md)

The patch never edits `mgs4.exe`. It loads through a reversible `winmm.dll`
proxy beside the executable. The optional direct-launch feature temporarily
replaces `Launcher/launcher.exe`, retains a private backup and restores it only
after verifying that the active file is our wrapper.

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
are welcome under the [MIT License](LICENSE). MinHook retains its BSD license;
see [third-party notices](THIRD_PARTY_NOTICES.md).

This is an unofficial community project. It contains no game files and is not
affiliated with or endorsed by KONAMI.
