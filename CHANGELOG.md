# Changelog

## v0.3.1-alpha.6 - experimental supersampling preview

- Removed the early central-camera/frustum rewrite after reproducing a native
  Windows aspect-ratio regression: characters became unnaturally tall and thin
  with supersampling both on and off. Restored the renderer projection behavior
  of the alpha.5 binary actually published and validated FOV 1.150 visually.
- Corrected the release history: the downloadable alpha.5 ASI used the
  renderer-only path despite later source/release notes claiming synchronized
  culling. Side-pop-in and extreme close-up continuity are again documented as
  open limitations rather than shipped fixes.
- Added optional internal-resolution supersampling while keeping the Windows
  output and game window at the configured physical resolution.
- Added a disabled-by-default configurator control with a live internal-size
  preview and an explicit performance, VRAM, stability and UI-size warning.
- Supersampling disabled now follows the published alpha.5 projection path;
  supersampling changes only internal/output resolution handling.
- Validated 3440x1440 physical output with 1.50x/5160x2160 internal rendering,
  windowed presentation and FOV 1.150 on native Windows.
- Isolated the depth-aware aiming-reticle boundary through same-session tests:
  stable at 3956x1656, flickering at exactly 4096 pixels wide and conditionally
  absent above it. The configurator and runtime log now advise keeping internal
  width below 4096 without enforcing a hard limit.
- Added render-extent unit tests, update/configuration smoke coverage and a
  simple manual INI fallback. Alpha.6 now includes a separate unvalidated
  Linux/Proton test package; alpha.5 remains the recommended stable package.

## v0.3.1-alpha.5 - synchronized FOV/culling refresh

> Packaging correction: the downloadable alpha.5 ASI did not contain the
> central camera/frustum hook described below. It retained the earlier
> renderer-only correction. These entries describe the attempted source change,
> not a capability that should be claimed for the published alpha.5 binary.

- Moved the primary ultrawide/FOV correction into the central camera builder at
  RVA `0x0b9bb0`, before combined view-projection matrices and CPU frustum
  planes are consumed.
- Rebuilt the primary and secondary combined matrices and all six normalized
  visibility planes from the corrected projection, eliminating the known
  render-FOV/culling mismatch that caused side pop-in above `1.000`.
- Retained the renderer projection hook only as a 16:9 fallback and prevented
  target-aspect camera matrices from receiving the FOV multiplier twice.
- Added projection/frustum unit coverage and runtime counters. Native Windows
  testing at 3440x1440 completed hundreds of synchronized camera builds without
  a crash or late fallback.
- Changed the recommended 21:9 framing to `FOVMultiplier=1.150`. `1.000` remains
  available as the original vertical-FOV option; `1.150` is visually wider and
  closely matches the established RPCS3 21:9 framing.
- Kept the reported 5120x2160 aiming-crosshair case open until that exact
  resolution and controller transition are reproduced.

- Refreshed the existing tag/assets in place while preserving the core/normal-
  FPS route and making corrected 120 FPS an optional additive installation.
- Delegated high-FPS timing to `cipherxof/MGSFPSUnlock` 0.1.0, thanked and
  credited upstream, disabled the old single-field writer and added direct
  upstream download plus pinned archive/ASI verification.
- Removed the unverified UI safe-area experiment from release builds and the
  configurator; original UI/effect behavior remains active.
- Fixed stale Steam libraries on disconnected drive letters aborting Windows
  discovery, and added locale-independent FOV parsing with actionable logs.
- Removed unconditional `gamemoderun` from generated Linux Gamescope commands,
  added a Gamescope availability check and published a separate Linux core
  package using the game's normal FPS behavior.
- Replaced the earlier projection-only FOV path that could reveal side
  pop-in/culling above `1.000`; projection and visibility bounds are now rebuilt
  together, and `1.150` is the recommended 21:9 framing.
- Removed standalone checksum files from GitHub release assets to keep the
  download list simple; local build checksums remain available to developers.
- Promoted the native-Windows ASI architecture validated in alpha.4 to `main`.
- Kept the tested `MGS4Ultra120.asi` and pinned Ultimate ASI Loader binaries
  unchanged.
- Added a ready-to-drag `Manual-Install` directory to the Windows ZIP, with
  matching loader, configuration and `scripts/MGS4Ultra120.asi` payloads.
- Simplified unsigned-binary guidance and documented source inspection, local
  compilation, ZIP/CMD setup and copy-only installation as equal choices.
- Recorded the user's successful manual focus-change validation.
- Accepted official `launcher_sv` variants that omit the redundant
  `WindowSizeW`/`WindowSizeH` fields when `ResolutionWindowW`/`H` are present.
- Synchronized every available official display field without inventing absent
  keys and retained strict requirements for exclusive fullscreen.
- Caught configurator save errors inside the GUI and stopped Easy Setup from
  reporting success when settings were not saved.
- Split Windows downloads into minimal manual, portable and complete ZIPs.

## v0.3.1-alpha.4 - Native Windows ASI release

- Split the Windows patch into pinned Ultimate ASI Loader `v9.7.4` as the x64
  `winmm.dll` proxy and `scripts/MGS4Ultra120.asi` as the project plugin.
- Added hash-verified upstream fetching, MIT attribution and redistributable
  metadata without committing the third-party executable to source control.
- Preserved the alpha.3 proxy build/test target as a reproducible legacy gate.
- Migrated installed alpha.3 ownership and original backups in place; retained
  supported INI values and the existing GUI/CMD configuration path.
- Added collision-safe ASI backup/restore and compatible-loader reuse so setup
  does not overwrite or uninstall an Ultimate ASI Loader owned by another mod.
- Added direct plugin load, loader-to-ASI integration, update/uninstall and
  external-loader coexistence smoke tests.
- Passed a native Steam launch at physical 3440x1440/60 with every stable hook,
  20 confirmed cross-monitor foreground changes, zero new dumps and zero new
  reliability/display events. G-SYNC remained disabled; no universal flicker
  fix is claimed.

## v0.3.1-alpha.3

- Replaced the two-export WinMM shim with a complete 181-export x64 proxy, so
  native Steam can resolve `waveOutGetDevCapsW` and all other WinMM imports.
- Fixed a native-Windows `0xc0000005` crash caused by restoring a transient
  non-executable protection after installing hooks in decrypted game code.
- Ignored unavailable/stale Steam library drives during automatic detection.
- Preserved supported user INI values across install/update operations.
- Enabled the reversible Unity-launcher bypass in the stable default profile
  and stopped duplicating bootstrap tokens on the child command line, avoiding
  Steam's repeated custom-arguments/launcher loop on affected clients.
- Added an unsigned persistent Windows setup EXE with Start-menu/desktop
  shortcuts and safe uninstall; retained the portable ZIP/CMD route.
- Simplified the Windows flow into one validated **Install/update and
  configure** action, added clear game-path status, recommended/default and
  save-and-close actions, using consistent English-only UI text to avoid
  locale-dependent encoding problems.
- Added a documented manual DLL+INI route compatible with the graphical
  configurator.
- Added separate Windows presentation profiles: a primary-monitor physical-size
  window is the stable default, while exclusive fullscreen is an explicitly
  warned advanced option.
- Synchronized the official launcher's full/window dimensions and mode before
  launch, with conditional restoration during uninstall so later user changes
  are preserved.
- Corrected the wrapper after native tracing showed that `-resolution` is a
  resolution slot, not the `WindowMode` flag; both stock presentation modes use
  slot `0` while `launcher_sv` controls windowed/fullscreen state.
- Isolated the moving red band/flicker to the G-SYNC/VRR presentation path on
  the tested mixed-refresh NVIDIA dual-monitor system. Auto HDR and windowed
  optimizations did not prevent recurrence, Windows recorded display WATCHDOG
  events, and ten focus transitions passed with G-SYNC disabled, including the
  final physical 3440x1440 test with every stable module enabled.
- Added explicit Auto HDR and NVIDIA multi-monitor warnings; the configurator
  documents the relevant settings but never changes Windows or driver state.
- Read the primary monitor's current physical mode through Win32 instead of
  DPI-scaled desktop bounds, so 3440x1440 at 125% is no longer saved as
  2752x1152.
- Added an installed-DLL hash marker so package updates retain the true
  pre-install backup and clean uninstall never restores an older project DLL.
- Added native proxy export/smoke checks and recorded the Windows failures as
  mandatory release gates for future CachyOS development.

## v0.3.1-alpha.2

- Fixed the Windows configurator crashing while initializing render widths
  above the WinForms default maximum of 100.
- Confirmed the width and height controls accept values through 16384,
  including 5120x1440 displays.

## v0.3.1-alpha.1

- Added a Windows easy-setup window launched by double-clicking
  `MGS4Ultra120-Setup.cmd`.
- Added automatic discovery of the game across the default Steam folder and
  additional libraries listed in `libraryfolders.vdf`.
- Combined reversible install/update, configuration and uninstall entry points
  in the easy-setup window.
- Rewrote the Windows guide so the normal path requires no terminal commands
  or manual file copying.
- Kept the in-progress proportional/anchored UI rewrite out of release builds;
  only the previously documented legacy 16:9 UI experiment remains available.

## v0.3.0-alpha.1 - 2026-08-28

- Added an independent controller-profile fix that preserves the game's native
  connected pad family without emulation, polling or a system input layer.
- Added an optional Steam-path direct-launch wrapper with reversible,
  update-aware launcher backups.
- Added a configurable event-driven hotkey to switch the FPS module between 60
  and experimental 120 FPS.
- Expanded both graphical configurators so ultrawide, FPS, controller fix,
  launcher and UI choices can be combined independently.
- Added a warned `AllowUnsupportedExecutable` override while retaining hook
  signature checks and the safe blocked default.
- Documented Bink video behavior, the planned anchored UI design, controller
  behavior and all supported option combinations.

## v0.2.1-alpha.1 - 2026-08-27

- Added engine-level Hor+ correction at arbitrary aspect ratios for the common
  DirectX 11/DirectX 12 projection path.
- Corrected native render-surface dimensions before renderer initialization;
  the patch is event-driven and does not rewrite memory every few milliseconds.
- Added a proportion-preserving FOV multiplier with `1.000` as the original
  vertical-FOV profile.
- Added independent ultrawide/FOV and FPS override modules. Either can be
  disabled completely, including ready-made `fps-only-120` and
  `ultrawide-only` profiles.
- Added stable 60 FPS, optional 30 FPS and experimental 120 FPS values. The 120
  mode remains experimental because scripted-scene stalls have been observed.
- Added an optional centered 16:9 D3D12 UI path. It remains experimental
  because some fullscreen effects share the identified shader.
- Added reversible Windows and Linux installers, graphical configurators,
  executable validation, and reversible Linux native/Gamescope launch modes.
- Added fail-closed compatibility checks for unknown executable updates.
