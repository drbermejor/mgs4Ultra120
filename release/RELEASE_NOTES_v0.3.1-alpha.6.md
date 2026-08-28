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

The first native-Windows validation passed at 3440x1440 output,
1.50x/5160x2160 internal rendering, windowed presentation and FOV 1.150. The
runtime reported the correct physical client size, 472 synchronized
camera/frustum updates and no late 16:9 fallback; the rendered result was
confirmed visually. This is not a full-playthrough or broad GPU certification.

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
internal resolution. Start with 1.50x on 3440x1440. Windowed presentation is the
only configuration tested so far. Saving requires an explicit warning
confirmation.

## Manual use

1. Close the game and extract the manual ZIP.
2. Copy `winmm.dll`, `mgs4_ultrawide.ini` and the included `scripts` folder into
   the `MGS4` folder containing `mgs4.exe`.
3. Open `mgs4_ultrawide.ini` and set:

   ```ini
   [Supersampling]
   SupersamplingEnabled=1
   RenderScale=1.50
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
- Exclusive fullscreen, HDR, VRR/G-SYNC, mixed-refresh multi-monitor systems
  and Proton require additional testing.
- Supersampling is off by default in every recommended profile.

The setup EXE is open-source but unsigned, so Windows or a browser may display
an unknown-publisher/reputation warning. The EXE is optional: use the portable
or manual ZIP if preferred. Download only from this official release and keep
antivirus enabled. Standalone checksum files are intentionally not included in
the download list.
