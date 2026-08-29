# MGS4 Ultra120 v0.3.4-alpha.1

This release keeps the existing ultrawide, controller, launcher, FPS and
supersampling features and adds two optional experiments:

- real-time cinematic FOV, with either the gameplay value (`inherit`) or a
  separate cinematic multiplier;
- a centered 16:9 HUD for DirectX 12.

Both new options are **disabled by default**. The normal recommended setup is
unchanged unless you deliberately enable them.

## Install

Choose one download:

- **Windows setup EXE**: easiest guided installation.
- **Windows portable ZIP**: extract it and run `MGS4Ultra120-Setup.cmd`.
- **Windows manual ZIP**: copy its contents beside `mgs4.exe`; no installer or
  script is run.
- **Windows complete ZIP**: portable setup, manual folder and documentation.
- **Linux/Proton tar.gz**: extract it, exit Steam and run
  `./MGS4Ultra120-Linux-Setup.sh`.

The Windows EXE is unsigned and may receive a SmartScreen or browser reputation
warning. It is optional: use the manual ZIP if you prefer to inspect and copy
the open-source payload yourself.

Remove older mixed or renamed builds before installing. Keep only one ASI
loader and do not mix ASIs from different MGS4 Ultra120 releases.

## Enable the experiments

Open the Windows or Linux configurator while the game is closed:

- enable **Experimental real-time cinematic FOV**;
- keep **Experimental native FOV** enabled because the cinematic route depends
  on it;
- keep **Use gameplay FOV for cinematics** selected, or enter a separate value;
- enable **Experimental centered 16:9 HUD** only when using DirectX 12.

Manual users can set `ExperimentalCinematicFOV=1` in
`mgs4_ultrawide.ini` and `Enabled=1` in
`mgs4_centered_hud_16x9.ini`.

Expanded cinematic framing can reveal actors, geometry or animation changes
slightly before the original shot intended. The HUD classifier is conservative,
but an uncommon menu, subtitle or prompt can still be misplaced. These are
experimental options, not requirements for the normal ultrawide fix.

## Return to the reference behavior

Close the game and disable the option that caused the problem. Disabling both
**Experimental real-time cinematic FOV** and **Experimental centered 16:9 HUD**
is sufficient to restore the reference rendering behavior; the disabled HUD
ASI exits before installing D3D12 hooks.

Windows command-line users can also apply `-Profile stable`. If a problem still
exists after both experiments are disabled, completely uninstall this build
and use the retained
[`v0.3.3-alpha.1` legacy fallback](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1).

The aiming-reticle limitation at supersampling internal widths around or above
4096 pixels is separate and remains unresolved. Keep supersampling disabled for
the safest configuration.

## Centered HUD screenshots

These ultrawide captures show the experimental HUD kept inside a centered 16:9
safe area while the 3D scene continues across the full output.

![Experimental centered 16:9 gameplay HUD](https://raw.githubusercontent.com/drbermejor/mgs4Ultra120/main/docs/images/v0.3.4-alpha.1-centered-16x9-hud-gameplay.jpg)

![Experimental centered 16:9 pause menu](https://raw.githubusercontent.com/drbermejor/mgs4Ultra120/main/docs/images/v0.3.4-alpha.1-centered-16x9-hud-pause-menu.jpg)
