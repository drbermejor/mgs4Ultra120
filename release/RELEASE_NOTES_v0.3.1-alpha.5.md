# MGS4 Ultra120 v0.3.1-alpha.5 - Windows ASI release

Alpha.5 promotes the ASI architecture validated in alpha.4 to the main Windows
line. The tested `MGS4Ultra120.asi` and pinned Ultimate ASI Loader binaries are
unchanged. Linux/Proton remains on its separate prior release line.

## Choose any installation route

- **Setup EXE:** automatic Steam detection, setup manager, desktop/Start-menu
  shortcuts, configurator and reversible uninstall.
- **ZIP/CMD:** extract the ZIP and double-click `MGS4Ultra120-Setup.cmd` to run
  the same readable PowerShell setup without the installer EXE.
- **Copy only:** drag everything inside `Manual-Install` into the `MGS4` folder
  containing `mgs4.exe`. The folder already contains the correct
  `scripts/MGS4Ultra120.asi` layout, root INI and loader.

The EXE is not digitally signed, so an unknown-publisher/reputation notice is
possible. The complete patch, installer, scripts and build instructions are
open source. Anyone can inspect the code, review the hash-pinned dependencies
and build an installer independently.

## Validation

- Native Steam launch at physical 3440x1440/60 with all stable hooks active.
- Ultimate ASI Loader, system WinMM and the project ASI confirmed in-process.
- Automated clean install, alpha.3/alpha.4 migration, update, backup/restore,
  uninstall, external-loader coexistence and incompatible-loader tests.
- Installer smoke test and exact manual-payload hash checks.
- Twenty automated focus changes plus the user's subsequent manual focus test
  completed without an apparent recurrence.

This remains a public alpha: 120 FPS and the centered UI option retain their
documented experimental limitations, and a full playthrough is not certified.
