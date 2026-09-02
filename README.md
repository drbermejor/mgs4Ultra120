# MGS4 Ultra120

Open-source ultrawide, FOV and controller-profile fixes for the Steam PC port
of *METAL GEAR SOLID 4*, with corrected 120 FPS support on Windows through
[cipherxof/MGSFPSUnlock](https://github.com/cipherxof/MGSFPSUnlock).

> **Public alpha.** `v0.3.4-alpha.7` targets the verified Steam executable.
> Other builds are blocked unless the user accepts the unsafe override. Back up
> saves and keep Steam's game files available for verification.

> **Recommended unified release.** Windows and Linux now share the validated
> native-camera FOV implementation. Optional supersampling remains experimental
> and disabled by default.

> **Native Centered HUD warning.** The old renderer-level HUD experiment has
> been removed and replaced by an earlier native-layout correction. The main
> HUD, menus, subtitles and guarded inventory previews now center much more
> consistently. Alpha.7 corrects the verified pause-map stream and adds guarded
> experimental adjustments for the live Codec render target and Mission
> Briefing compositors. The Codec frame is centered, but its live in-engine
> content can remain horizontally compressed; Mission Briefing control and
> ticker text can still overflow the safe area. The option remains
> **experimental and disabled by default** because coverage is deliberately
> limited to exact observed routes. Disabling it does not affect ultrawide,
> FOV, FPS, supersampling or the reticle fix.

## What the Windows release installs

| Component | Purpose |
|---|---|
| `winmm.dll` | Pinned Ultimate ASI Loader v9.7.4 |
| `scripts/MGS4Ultra120.asi` | Ultrawide/Hor+, FOV and controller-profile fixes |
| `mgs4_ultrawide.ini` | Shared MGS4Ultra120 settings |
| `scripts/MGS4NativeCenteredHUD.asi` | Optional experimental native 16:9 HUD correction |
| `mgs4_native_centered_hud.ini` | Independent HUD switch; disabled by default |
| `scripts/MGSFPSUnlock.asi` | Corrected high-frame-rate implementation by cipherxof |
| `scripts/MGSFPSUnlock.ini` | Persistent FPS target; defaults to 120 |

Our old single-value FPS override is disabled and no longer writes the game's
FPS state. MGSFPSUnlock owns frame-rate timing and supplies separate fixes for
camera movement, character control, polygon demos, physics, ragdolls, cloth,
hair/bandana, wind and SPURS tasks. This avoids two plugins fighting over the
same setting.

MGSFPSUnlock is an independent source-published project. Because its repository
currently does not declare a redistribution license, its binary is **not
repackaged** in our downloads. Guided setup downloads version 0.1.0 directly
from its official GitHub release, requires the pinned archive and ASI SHA-256
values to match, and installs only its ASI and INI. Thank you to
[cipherxof](https://github.com/cipherxof) for the substantially improved FPS
unlock implementation.

## Rendering status

- 3440x1440 real-time 3D rendering is validated as correctly proportioned and
  Hor+ rather than a stretched 16:9 image.
- `FOVMultiplier=1.200` is the tested 21:9 recommendation under the corrected
  single-owner model. `1.000` preserves the
  original vertical FOV but frames Snake more tightly in the tested view.
- Native FOV is still experimental. Disable it independently in the
  configurator if a new stability or scene-specific issue appears; Hor+ remains
  active with the original vertical FOV.
- Real-time cinematic FOV and Native Centered HUD are included as separate
  disabled options. The cinematic multiplier can inherit gameplay FOV or use
  an independent value. Native Centered HUD works on native layout data rather
  than graphics-API draws. Its pause-map correction and partial live Codec and
  Mission Briefing adjustments use exact native identities and fail closed on
  unknown states.
- Disabling `ExperimentalCinematicFOV` and the Native Centered HUD `Enabled`
  switch restores the reference rendering behavior without reinstalling. The
  HUD companion exits before installing any hooks while disabled.
- Pre-rendered Bink video is unchanged.
- FOV is applied once to the native camera-builder input. The game then creates
  its own projection, combined matrices and frustum planes from that corrected
  input; the final renderer hook changes aspect only. This avoids both the old
  render/culling mismatch and the withdrawn `v0.3.1-alpha.6` double
  transformation.
- Native Windows validation at 3440x1440 completed with native mode active, no
  fallback, natural object proportions, an undistorted WeaponWindow and clean
  problematic cinematics.
- Native FOV remains experimental. For maximum stability, leave it disabled;
  values above the tested `1.200` recommendation are allowed but untested and
  may expose geometry or animation transitions early, produce unusual framing
  or cause other scene-specific problems. Use them under your own responsibility.
- The aiming-reticle overflow at internal widths of 4096 pixels or more is
  fixed at its four native X/Y coordinate conversions. Native Windows gameplay
  validation confirmed the reticle working at 3440x1440 output with a
  5160x2160 internal render. Supersampling remains experimental because of its
  performance, VRAM and presentation costs, not because of that old width
  boundary.

## Screenshots

**Reticle fix at 5160x2160 internal rendering**

The output remains 3440x1440 while experimental 1.50x supersampling renders
internally at 5160x2160. The aiming reticle remains visible after removing the
four signed 16-bit coordinate conversions.

![Aiming reticle visible at 3440x1440 output and 5160x2160 internal rendering](docs/images/reticle-3440x1440-output-5160x2160-internal.jpg)

Captured during the final `v0.3.3-alpha.1` Windows validation at 3440x1440 with
`FOVMultiplier=1.200`.

**Gameplay**

![Correctly proportioned 3440x1440 gameplay](docs/images/v0.3.3-alpha.1-gameplay-3440x1440.png)

**WeaponWindow**

![Undistorted WeaponWindow at 3440x1440](docs/images/v0.3.3-alpha.1-weapon-window-3440x1440.png)

### Experimental Native Centered HUD (`v0.3.4-alpha.7`)

The current native-layout path keeps gameplay widgets in a centered 16:9 safe
area while the 3D scene continues to use the full ultrawide output.

**Centered gameplay HUD**

![Native Centered HUD during 3440x1440 gameplay](docs/images/v0.3.4-alpha.6-native-centered-hud-gameplay.jpg)

**Corrected pause map**

The map plane and its live markers retain their intended aspect inside the
centered menu. The surrounding legend follows the normal centered layout.

![Correctly proportioned pause map inside the centered menu](docs/images/v0.3.4-alpha.7-native-centered-hud-map.png)

**Centered Codec frame; live-feed limitation remains**

The Codec frame and controls use the centered 16:9 canvas, and static or
prerendered content follows it. The observed live in-engine 3D feed can still
remain horizontally compressed. The screenshot below documents that current
experimental state; it is not evidence of a fully corrected live feed.

![Centered Codec frame with horizontally compressed live content](docs/images/v0.3.4-alpha.7-native-centered-hud-codec.png)

**Partially adjusted Mission Briefing composition**

The guarded briefing hooks reposition the observed root, persistent view
rectangles and four auxiliary surfaces while leaving full-output clears
untouched. The composition is not fully contained: control and ticker text can
still extend beyond or be clipped by the centered safe canvas.

![Mission Briefing with remaining control-text overflow](docs/images/v0.3.4-alpha.7-native-centered-hud-briefing.jpg)

## Windows downloads

Use the
[latest v0.3.4-alpha.7 release](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.4-alpha.7)
and choose one package:

1. **Setup EXE** — guided Steam detection, configuration, shortcuts and safe
   uninstall. It requires internet access once to fetch the verified
   MGSFPSUnlock 0.1.0 package.
2. **Portable ZIP** — extract it, then run `MGS4Ultra120-Setup.cmd`. It provides
   the same guided installation without registering a persistent application.
3. **Manual ZIP** — the most transparent copy-only route. It runs no script,
   but corrected 120 FPS requires two additional files from the official
   MGSFPSUnlock 0.1.0 ZIP; see the short instructions below.
4. **Complete ZIP** — portable setup, manual folder, source-facing notices and
   all documentation in one archive.
5. **16:9 supersampling ZIP** — portable setup preconfigured for 1920x1080
   output with a 3840x2160 internal render. Ultrawide, FOV changes and the
   Native Centered HUD start disabled; the controller fix and optional
   corrected FPS support remain available.

The EXE is unsigned, so SmartScreen or a browser may show an
unknown-publisher/reputation warning. The EXE is optional. Download only from
the official release and keep antivirus enabled. All project code and build
instructions are public and can be reviewed or built independently.

Optional supersampling is included in the same release, remains disabled by
default and is intended only for users who explicitly want to render above
their physical output resolution. See
[experimental supersampling](docs/EXPERIMENTAL_SUPERSAMPLING.md).

The real-time cinematic FOV and Native Centered HUD are also disabled by
default. Native HUD centering is substantially more complete than the removed
renderer-level experiment and includes a verified pause-map correction plus
guarded, partial Codec and Mission Briefing adjustments. The live Codec 3D feed
can remain compressed, and briefing text can overflow. Leave it disabled unless
you accept these and other exceptions in untested acts, languages, content
types and transitions.
If either experiment causes a problem, close the game and disable that option.
For a complete known fallback, the previous
[`v0.3.3-alpha.1`](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1)
release remains available.

### Brief manual installation

Remove old MGS4 Ultra120 builds first; do not retain another Ultimate ASI Loader
proxy or a renamed old project ASI. Close the game, extract the manual ZIP and copy its contents into the folder
that directly contains `mgs4.exe`:

```text
MGS4\winmm.dll
MGS4\mgs4_ultrawide.ini
MGS4\mgs4_native_centered_hud.ini
MGS4\scripts\MGS4Ultra120.asi
MGS4\scripts\MGS4NativeCenteredHUD.asi
```

For corrected 120 FPS, download
[MGSFPSUnlock 0.1.0](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0),
open its ZIP and copy only these two files into the same `MGS4\scripts` folder:

```text
MGSFPSUnlock\scripts\MGSFPSUnlock.asi
MGSFPSUnlock\scripts\MGSFPSUnlock.ini
```

Do **not** copy its `d3d11.dll`, `winhttp.dll` or `wininet.dll`; our package
already provides the required ASI loader. Confirm that its INI contains
`TargetFrameRate = 120`, then launch normally through Steam. The manual ZIP
does not install the optional launcher bypass; choose the game language in the
official Unity launcher. The `Language=` INI setting controls only the optional
wrapper installed by guided setup.

See [Windows installation](docs/INSTALL_WINDOWS.md) and
[manual installation](docs/MANUAL_INI.md) for backup and removal details.

## Windows defaults and known display issue

The configurator selects the primary monitor's physical resolution, native-size
windowed presentation, FOV 1.200, corrected 120 FPS and Unity-launcher bypass.
The controller-profile workaround is opt-in because it can interfere with
keyboard/mouse and hybrid controller plus mouse/gyro input. Exclusive
fullscreen is an advanced option.

**Controller notice:** new installations keep the controller-profile workaround
disabled. Existing explicit choices are preserved during managed updates. If
you need it, enable **Controller profile fix** in the configurator. Manual users
can instead set `ControllerProfileFixEnabled=1` under `[Input]` in
`mgs4_ultrawide.ini`.

On one mixed-refresh NVIDIA multi-monitor system, focus changes produced a red
sweep/flicker and Windows display WATCHDOG reports. Testing was clean after
G-SYNC/VRR was disabled, but that is not a universal diagnosis. The
configurator warns and never changes driver or Windows display settings. See
[Windows troubleshooting](docs/TROUBLESHOOTING_WINDOWS.md).

## Linux / Proton

The `v0.3.4-alpha.7` Linux package uses the same architecture as Windows:
pinned Ultimate ASI Loader plus separate `MGS4Ultra120.asi` and optional
`MGSFPSUnlock.asi` plugins. Easy Setup downloads MGSFPSUnlock 0.1.0 from its
official release, verifies its hashes and applies the Wine `PAGE_WRITECOPY`
compatibility byte locally; its unlicensed binary is never redistributed.

The Linux GUI configures resolution, FOV, ultrawide, supersampling, 30/60/120
FPS, controller-profile correction, launcher bypass and native/Gamescope
fullscreen. Easy Setup creates a persistent configurator shortcut on the
desktop and in the application menu. It discovers native and Flatpak Steam
libraries, offers a folder selector when needed and remembers the selected
game directory for later configuration and uninstall. GE-Proton10-34, DX12,
3440x1440 Hor+ with 3956x1656 internal
supersampling, all FPS timing hooks and a true 3440x1440 Gamescope client were
validated. See [Linux installation](docs/INSTALL_LINUX.md) and
[corrected FPS on Proton](docs/PROTON_FPS.md).

## Technical outline

MGS4Ultra120 adjusts the native camera input scale before the game builds its
dependent projection and visibility state:

```text
adjusted_camera_scale = original_camera_scale / FOVMultiplier
new_m00 = sign(m00) * abs(native_m11) / target_aspect
```

The withdrawn alpha.6 candidate edited already-built matrices and reconstructed
camera/frustum state after return, causing an additional transformation. The
new path changes only the original scalar input. The game remains the sole
owner of its matrices and frustum planes, while the common renderer setter is
an aspect-only safety net in native mode.

The patch never edits `mgs4.exe`. The optional direct-launch wrapper backs up
`Launcher/launcher.exe`, reproduces the official launcher's child-process
command line, and restores the original only when ownership hashes still match.

Further reading:

- [Configuration](docs/CONFIGURATION.md)
- [Experimental supersampling](docs/EXPERIMENTAL_SUPERSAMPLING.md)
- [Controller profile fix](docs/CONTROLLER_FIX.md)
- [Direct-launch wrapper](docs/LAUNCHER_WRAPPER.md)
- [UI and video status](docs/UI_AND_VIDEO.md)
- [Technical notes](docs/TECHNICAL.md)
- [Development and reproducible builds](docs/DEVELOPMENT.md)

MGS4 Ultra120 is MIT-licensed. Third-party components retain their own terms;
see [third-party notices](THIRD_PARTY_NOTICES.md). No game files are included.
This project is not affiliated with or endorsed by KONAMI.
