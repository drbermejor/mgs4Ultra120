# Optional direct-launch wrapper

The Unity launcher can be skipped without launching `mgs4.exe` from an
untracked desktop command. When enabled, Steam still starts
`Launcher/launcher.exe`; MGS4 Ultra120's small wrapper occupies that path,
writes the bootstrap parameters needed by the game, starts `MGS4/mgs4.exe`,
waits for it and returns its exit code to Steam.

This retains the normal Steam application launch relationship for the overlay,
play-time accounting and the game's normal save location. Steam Cloud behavior
is not reimplemented by the wrapper and still depends on Steam and the game.
Cloud transfer has not yet been certified on every platform, so keep save
backups during the alpha.

The feature is off by default. Enable **Skip the Unity launcher** in the
configurator and choose the desired language. No custom Steam target or
desktop shortcut is required.

## Reversibility and updates

The configurator backs up the current launcher under
`MGS4/.mgs4ultra120-backup/launcher.exe.preinstall`. It restores that file only
when the active launcher hash matches the packaged wrapper. If Steam or another
tool changes the launcher, MGS4 Ultra120 preserves both files and warns rather
than overwriting an unknown/newer launcher.

Steam may replace the wrapper during a game update. Rerun the configurator to
enable it again; the newly installed launcher becomes the newest restore
target and older backup generations are retained with UTC timestamp suffixes.

The wrapper log is `Launcher/mgs4_direct_wrapper.log`.
