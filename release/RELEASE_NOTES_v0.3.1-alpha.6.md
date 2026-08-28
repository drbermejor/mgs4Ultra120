# MGS4 Ultra120 v0.3.1-alpha.6 - experimental supersampling preview

> **Experimental Windows prerelease.** Supersampling is disabled by default.
> Use [v0.3.1-alpha.5](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.1-alpha.5)
> if you want the validated ultrawide/FOV/120 FPS release without this preview.

This preview adds optional resolution-independent supersampling. `Width` and
`Height` remain the physical output and game-window size, while `RenderScale`
raises the internal render resolution before DXGI presents the complete frame
at the physical size. It does not require AMD VSR, NVIDIA DSR or a Windows
desktop-resolution change.

Examples:

| Physical output | Scale | Internal render |
| --- | ---: | --- |
| 1920x1080 | 2.00 | 3840x2160 |
| 2560x1080 | 2.00 | 5120x2160 |
| 3440x1440 | 1.50 | 5160x2160 |
| 3440x1440 | 2.00 | 6880x2880 |

Native-Windows testing confirms that supersampling keeps a real 3440x1440
window while rendering the world internally at 5160x2160, with synchronized
camera/frustum updates and no late 16:9 fallback. Aiming-crosshair behaviour at
high internal resolutions is still under investigation. It remains visible at
3956x1656. At 4128x1728 it appears or disappears depending on the depth of the
aimed point; it was also captured missing at 5160x2160. This points to the
high-width projection/update path of the depth-aware reticle rather than FOV,
world culling or the physical monitor. Keep internal width at or below 4096 for
normal gameplay until the targeted UI defect is fixed.

Alpha.6 also contains the public alpha.5 fixes, including synchronized FOV and
world culling, continuous FOV through tight in-engine close-ups, the controller
profile fix, optional corrected 120 FPS setup and the Unity-launcher bypass.

## Downloads

- `MGS4Ultra120-v0.3.1-alpha.6-windows-setup.exe` - easiest guided setup,
  configurator, shortcuts and reversible uninstall.
- `MGS4Ultra120-v0.3.1-alpha.6-windows-portable.zip` - the same setup through
  the readable `MGS4Ultra120-Setup.cmd` route without registering an app.
- `MGS4Ultra120-v0.3.1-alpha.6-windows-manual.zip` - minimal copy-only files;
  no script is executed.
- `MGS4Ultra120-v0.3.1-alpha.6-windows-complete.zip` - portable setup, manual
  tree, documentation and notices together.

There is no alpha.6 Linux package because supersampling has not been validated
under Proton. Linux users should remain on the separate alpha.5 package.

## Guided use

Install or update with the EXE or portable CMD, open the configurator, enable
**Experimental supersampling**, choose a scale and review the calculated
internal resolution. At 3440x1440, 1.15x/3956x1656 is the conservative starting
point currently confirmed with a visible crosshair. Windowed presentation is
the only configuration tested so far. Saving requires an explicit warning
confirmation.

## Manual use

1. Close the game and extract the manual ZIP.
2. Copy `winmm.dll`, `mgs4_ultrawide.ini` and the included `scripts` folder into
   the `MGS4` folder containing `mgs4.exe`.
3. Open `mgs4_ultrawide.ini` and set:

   ```ini
   [Supersampling]
   SupersamplingEnabled=1
   RenderScale=1.15
   ```

4. Keep `Width` and `Height` at the physical output resolution and launch
   normally through Steam.
5. If startup fails, close the game and set `SupersamplingEnabled=0` to return
   to the stable rendering path.

## Important limitations

- Rendering cost and approximate render-target memory grow with the square of
  the scale: 1.50x uses 2.25x as many pixels and 2.00x uses 4x.
- Excessive settings may exhaust VRAM, reduce performance, crash the game or
  reset the graphics driver. The patch warns about unusual values but does not
  guess a safe limit for each GPU.
- The complete frame is downsampled. HUD text and the Steam overlay may appear
  smaller because world and UI rendering are not separated.
- **Known crosshair defect:** above 4096 internal pixels wide, the depth-aware
  aiming reticle can appear or disappear depending on the distance of the aimed
  point. This is reproduced at 4128x1728 and captured at 5160x2160. Keep the
  calculated internal width at or below 4096 for normal gameplay.
- Exclusive fullscreen, HDR, VRR/G-SYNC, mixed-refresh multi-monitor systems
  and Proton require additional testing.
- Supersampling is off by default in every recommended profile.

The setup EXE is open-source but unsigned, so Windows or a browser may display
an unknown-publisher/reputation warning. The EXE is optional: use the portable
or manual ZIP if preferred. Download only from this official release and keep
antivirus enabled. Standalone checksum files are intentionally not included in
the download list.
