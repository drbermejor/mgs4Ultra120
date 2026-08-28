# MGS4 Ultra120 v0.3.1-alpha.5

Alpha.5 is the recommended release for normal ultrawide play. It provides:

- Native ultrawide/Hor+ rendering and configurable FOV.
- Recommended `FOVMultiplier=1.150` for 21:9; `1.000` preserves the original
  vertical FOV.
- Controller-profile correction and optional Unity-launcher bypass.
- Normal 30/60 FPS operation and optional corrected 120 FPS setup through
  [cipherxof/MGSFPSUnlock 0.1.0](https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0).
- Separate Windows and Linux/Proton packages.

## Release-record correction

The downloadable alpha.5 Windows ASI uses the visually validated renderer-level
projection correction. A later source revision and previous release notes
incorrectly claimed that the published package also contained a central
camera/frustum rewrite with synchronized CPU culling. It did not.

That early rewrite first reached users in the original alpha.6 package and was
withdrawn after native Windows testing reproduced distorted tall/thin character
proportions. Alpha.5 itself remains the known-good normal release.

FOV values above `1.000` can still expose side pop-in because CPU visibility
bounds are not expanded, and extreme in-engine close-ups may briefly use the
original framing. These are open limitations, not fixed alpha.5 features.

## Windows downloads

- `windows-setup.exe`: guided install, configuration, shortcuts and uninstall.
- `windows-portable.zip`: extract and run `MGS4Ultra120-Setup.cmd`.
- `windows-manual.zip`: copy-only `winmm.dll`, INI and `scripts` folder.
- `windows-complete.zip`: portable and manual layouts plus documentation.

The installer is open source but unsigned. The EXE is optional; use the manual
ZIP if preferred.

## Linux

Use the separate alpha.5 Linux tarball for the established Proton path. It uses
the game's normal FPS behavior and does not require `gamemoderun`.

The patch does not modify `mgs4.exe` and contains no game files.
