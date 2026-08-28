# MGS4 Ultra120 v0.3.1-alpha.3

Critical Windows-native repair and direct installer release. This release
supersedes alpha.2 for Windows users.

## Fixed

- Native Steam no longer fails with a missing `waveOutGetDevCapsW` entry point.
  The local WinMM proxy now mirrors all 181 exports from 64-bit system WinMM.
- The game no longer crashes with `0xc0000005` after hook initialization on
  native Windows. Successfully hooked protected-code pages remain executable.
- Automatic Steam discovery skips stale/offline libraries such as a missing
  `E:` drive instead of aborting setup.
- The alpha.2 WinForms range correction is included, so 3440x1440 and widths
  through 16384 initialize correctly.
- Updating the package preserves supported user settings in the installed INI.
- The direct wrapper now keeps launch tokens exclusively in the game's official
  `mgs4_param` bootstrap. It no longer repeats them on `mgs4.exe`'s command line,
  which caused a Steam confirmation/launcher loop on affected clients.
- Windows now defaults to a primary-monitor physical-size window. On the tested
  3440x1440 display it covered the complete monitor without a title bar and
  passed repeated dual-monitor focus changes with G-SYNC disabled.
- Exclusive fullscreen remains an advanced option. The configurator first
  synchronizes all official-launcher dimensions, records the originals for
  conditional uninstall restoration and displays a mode-specific warning.
- Native tracing corrected a misleading launcher token: `-resolution 0` is a
  resolution slot used by the stock launcher even with `WindowMode=1`.
  Alpha.3 changes presentation only through the official saved mode field.
- DLL ownership is now recorded by hash. Updating from another alpha preserves
  the true pre-install DLL backup, and uninstall no longer leaves or restores
  an obsolete MGS4 Ultra120 proxy.
- Primary-monitor auto-detection now reads the physical Windows display mode;
  3440x1440 at 125% scaling is saved as 3440x1440, not the DPI-scaled
  2752x1152 desktop size.

## Windows installation

- `windows-setup.exe` is a persistent per-user installer with Start-menu and
  desktop shortcuts plus safe uninstall.
- The English-only setup manager/configurator validate the detected path,
  provide a single primary install-and-configure action and finish with an
  explicit **Save and close** step.
- The installer is intentionally unsigned. Verify the adjacent `.sha256`,
  obtain it only from this repository's Releases page and do not disable
  SmartScreen or antivirus globally.
- The portable Windows ZIP and `MGS4Ultra120-Setup.cmd` remain supported.
- A manual `winmm.dll` + `mgs4_ultrawide.ini` copy route is now documented;
  the GUI and text editor operate on the same INI.

The stable profile detects the primary monitor's physical resolution and uses
windowed/native presentation, FOV 1.000, 60 FPS, original UI,
controller-profile correction and the reversible Unity-launcher bypass. An
unexpected or repeating Steam custom-arguments prompt should be cancelled and
reported; alpha.3's wrapper no longer requires those visible child arguments.

120 FPS and the centered D3D12 UI mode remain experimental. This alpha is
verified against the supported Steam executable but not a full playthrough.
Changing focus reproduced a red sweep/flicker on a mixed-scaling 240/144 Hz RTX
4090 dual-monitor setup, including one light windowed occurrence while every
patch module was off; one run required `Win+Ctrl+Shift+B` after exit. Auto HDR
and windowed-game optimizations did not prevent recurrence, and Windows logged
display `WATCHDOG` `LiveKernelEvent` reports. With G-SYNC disabled, ten focus
transitions passed, including five at physical 3440x1440/60 with every stable
module enabled. The configurator warns on NVIDIA multi-monitor systems but does
not alter driver or Windows display settings. This release publishes Windows
setup/ZIP assets only; Linux remains on its separately validated prior release
line.
