# MGS4 Ultra120 v0.3.1-alpha.1

This revision makes the Windows installation substantially simpler. The patch
modules are unchanged from v0.3.0-alpha.1; the unfinished proportional UI
rewrite is deliberately **not** included.

## Windows: new easy setup

1. Download and extract the Windows ZIP.
2. Exit the game.
3. Double-click `MGS4Ultra120-Setup.cmd`.
4. Click **Install / update** in the detected game folder.
5. Select any combination of fixes in the configurator and click
   **Save settings**.

The setup searches the normal Steam location and every additional Steam
library listed in `libraryfolders.vdf`. A Browse button remains available for
unusual layouts. The same window can reopen the configurator or uninstall the
patch and restore pre-installation files.

## Included modules

- Configurable ultrawide output and engine-level Hor+ projection.
- Independent 30/60/experimental 120 FPS override and F10 60/120 toggle.
- Optional controller-profile correction.
- Optional direct-launch wrapper using the normal Steam launch path.
- Legacy centered 16:9 UI experiment (D3D12, off by default).

## Important warnings

- 120 FPS is experimental. A scripted intro has stalled while audio continued;
  physics, QTEs and script timing have not passed a full playthrough.
- Leave the legacy UI experiment disabled for the stable rendering profile. It
  can interfere with full-screen effects.
- Pre-rendered videos are not reformatted by this release.
- Unknown game executables are warned about and blocked unless the user enables
  the explicit unsafe override. Continuing can crash the game.
- This is an unofficial community alpha. Back up saves before testing.
