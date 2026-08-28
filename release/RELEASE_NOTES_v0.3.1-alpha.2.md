# MGS4 Ultra120 v0.3.1-alpha.2

Windows configurator hotfix. No patch DLL behavior changed, and the unfinished
proportional UI rewrite remains excluded.

## Fixed

- The configurator could assign `Value=3440` before expanding a WinForms
  numeric control's default maximum of 100, causing it to abort immediately.
- Numeric ranges are now initialized in a deterministic order and accept
  widths and heights up to 16384, including 5120x1440.

## Easy Windows installation

1. Download and extract the Windows ZIP.
2. Close the game.
3. Double-click `MGS4Ultra120-Setup.cmd`.
4. Click **Install / update**.
5. Enter the display resolution, choose any combination of fixes and click
   **Save settings**.

120 FPS and the legacy centered 16:9 UI mode remain experimental. Use 60 FPS
and leave the UI experiment disabled for the stable rendering profile.
