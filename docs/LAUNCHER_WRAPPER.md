# Optional direct-launch wrapper

The Unity launcher can be skipped without launching `mgs4.exe` from an
untracked desktop command. When enabled, Steam still starts
`Launcher/launcher.exe`; MGS4 Ultra120's small wrapper occupies that path,
uses the official `mgs4_param` bootstrap format needed by the game, starts
`MGS4/mgs4.exe` without duplicating those tokens on its command line,
waits for it and returns its exit code to Steam.

This retains the normal Steam application launch relationship for the overlay,
play-time accounting and the game's normal save location. Steam Cloud behavior
is not reimplemented by the wrapper and still depends on Steam and the game.
Cloud transfer has not yet been certified on every platform, so keep save
backups during the alpha.

The feature is on in the default stable profile to avoid Unity-launcher
confusion. Clear **Skip the Unity launcher** in the configurator to restore the
original launcher. No custom Steam target is required.

The original Unity launcher can cause Steam to show **Start game with custom
arguments**. Older MGS4 Ultra120 wrappers duplicated the same bootstrap tokens
on the child command line and could enter a prompt/launcher loop on some Steam
clients. Alpha.3 keeps the tokens only in `mgs4_param`; its direct-launch child
therefore has no custom command-line arguments. Cancel any unexpected Steam
argument prompt and report it with `Launcher/mgs4_direct_wrapper.log`.

## Windows presentation settings

`DisplayMode` is applied through `WindowMode` in the official `launcher_sv`
settings, not through the similarly named `-resolution` argument. Native
tracing proved that Unity emits `-resolution 0` in both a windowed launch and
the original fullscreen profile; that token is an independent resolution slot.
Alpha.3 therefore preserves slot `0` for both presentation modes.

The configurator synchronizes the official launcher's full, window and window-
size fields to the selected physical resolution before either mode. This avoids
the inconsistent 2560x1440-fullscreen/3440x1440-window state found on the test
machine. Uninstall conditionally restores the recorded originals. Linux keeps
its existing launch-mode handling and does not consume these Windows-only INI
keys.

## Reversibility and updates

The configurator backs up the current launcher under
`MGS4/.mgs4ultra120-backup/launcher.exe.preinstall`. It restores that file only
when the active launcher hash matches the packaged wrapper. If Steam or another
tool changes the launcher, MGS4 Ultra120 preserves both files and warns rather
than overwriting an unknown/newer launcher.

The installed wrapper hash is recorded separately. Updating MGS4 Ultra120 can
therefore recognize and replace its own older wrapper without ever reclassifying
that file as the original Unity launcher.

Steam may replace the wrapper during a game update. Rerun the configurator to
enable it again; the newly installed launcher becomes the newest restore
target and older backup generations are retained with UTC timestamp suffixes.

The wrapper log is `Launcher/mgs4_direct_wrapper.log`.
