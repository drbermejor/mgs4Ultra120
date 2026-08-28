# Troubleshooting

## `3440 is not a valid value`

This was a Windows configurator bug in an older package. Delete the extracted
old tool folder, download the newest asset whose name ends in `-windows.zip`
from [Releases](https://github.com/drbermejor/mgs4Ultra120/releases), extract
it, and run `MGS4Ultra120-Setup.cmd`. Do not use GitHub's **Source code** ZIP.
The fix is included in `v0.3.1-alpha.2` and later.

## Steam asks about custom arguments

The original Unity launcher asks Steam to start `mgs4.exe` with the language,
region, resolution, controller and launcher-root arguments selected in the
launcher. Steam may show those arguments for confirmation. A command ending in
a complete quoted path such as:

```text
-launcherroot "E:\\SteamLibrary\\steamapps\\common\\METAL GEAR SOLID 4\\launcher"
```

is expected. Choose **Continue** to proceed. `-ctrltype AUTO` identifies the
original launcher path; it does not mean that the main DLL patch failed.

If approving the arguments opens the Unity launcher again and produces the
same prompt repeatedly, stop the loop and enable **Skip the Unity launcher** in
the MGS4 Ultra120 configurator. Steam then starts the packaged wrapper at its
normal launcher path, and the wrapper starts `mgs4.exe` directly instead of
submitting another child-launch request to Steam.

The optional direct-launch wrapper is disabled by default. It is a separate
module from the main `winmm.dll` patch and can be enabled under **Skip the Unity
launcher** in the configurator.

## Confirm which component ran

- Main ultrawide/FPS/input-profile DLL: `MGS4\\mgs4_ultrawide.log`
- Optional direct-launch wrapper: `Launcher\\mgs4_direct_wrapper.log`

The main log should contain `Configuration:` and the enabled hook messages. If
it is missing, confirm that `winmm.dll` and `mgs4_ultrawide.ini` are beside
`mgs4.exe`, not beside the top-level Unity launcher.

Repeated `Configuration:` lines without the later hook messages can mean that
`mgs4.exe` is being started and immediately sent back to the original launcher.
Use the direct-launch option above, then check the wrapper log.

## What to attach to an issue

Close the game, then attach:

- the package version and exact downloaded asset name;
- Windows version, GPU, DX11/DX12 choice and display resolution;
- `mgs4_ultrawide.log`;
- `mgs4_direct_wrapper.log` only if direct launch is enabled;
- a screenshot of the complete error or Steam argument dialog.

Do not include save files or other personal data.
