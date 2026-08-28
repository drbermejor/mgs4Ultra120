# Windows troubleshooting

## `waveOutGetDevCapsW` entry-point error

Do not use v0.3.1-alpha.1 or alpha.2 on native Windows. Their two-export WinMM
shim was insufficient for Steam's native client DLL. Install alpha.3 or newer,
whose proxy mirrors the complete 64-bit WinMM export table.

The alpha.4 ASI release no longer implements WinMM forwarding in project code.
It uses pinned Ultimate ASI Loader `v9.7.4` as the proxy and loads
`scripts/MGS4Ultra120.asi`. If alpha.4 reports a loader problem, verify both
files and their published hashes rather than copying an alpha.3 DLL over it.

## Configurator says `WindowSizeW` is missing

Download the refreshed alpha.5 package. Some valid official `launcher_sv` variants omit
`WindowSizeW`/`WindowSizeH` and use `ResolutionWindowW`/`ResolutionWindowH`
instead. Older configurators treated every field as mandatory, aborted the
save and then allowed Easy Setup to display a false completion message.

The refreshed alpha.5 accepts either complete window-size pair, updates all available
official fields and saves the patch INI normally. It also handles save errors
inside the GUI and only reports setup completion after a successful save.

## `Cannot find drive E:` during setup

The original alpha.3 fix still used `Join-Path` while reading
`libraryfolders.vdf`, so PowerShell could abort before reaching a valid library.
Redownload the refreshed alpha.5 package: discovery now treats an offline
library as a missing path and continues. If the game itself is on a disconnected
drive, reconnect it or use **Browse** to select the current MGS4 folder.

## Manual ZIP log says `invalid enabled module configuration`

The published template contains valid values, but older builds did not log the
path or rejected values and parsed decimal FOV through process locale state.
The refreshed ASI uses a locale-independent parser, accepts `1.000` or `1,000`
and logs the exact INI path, width, height and FOV if validation fails. Confirm
that `mgs4_ultrawide.ini` is directly beside `mgs4.exe`, not one folder deeper.

## Steam repeats the custom launch-arguments prompt

The original Unity launcher and pre-alpha.3 direct wrapper can both expose the
game tokens on the `mgs4.exe` command line. Some Steam clients then return to
`launcher.exe` after **Continue**, creating an endless confirmation loop.
Alpha.3 enables the direct wrapper by default and passes the tokens only through
the game's official `mgs4_param` bootstrap. Install/update alpha.3, save with
**Skip the Unity launcher** enabled, and launch again. Do not keep approving a
repeating prompt. If it still appears, cancel and attach
`Launcher/mgs4_direct_wrapper.log` to an issue.

The original reporter of Issue #2 did not post a final alpha.3 confirmation.
Current release confidence comes from native local validation and automated
wrapper install/launch/update/restore tests. Manual-ZIP installation does not
enable the wrapper; use portable setup once if this Steam-specific loop occurs.

## A monitor flickers or shows a moving colored band

During native-Windows testing on an RTX 4090 with 3440x1440 and 2560x1440
monitors at 240/144 Hz, different scaling factors and HDR enabled, changing
which monitor had focus reproduced a moving red sweep/flicker. It was strongest
in exclusive fullscreen but a lighter occurrence was also seen in a
native-size window with every patch module disabled. One run left the secondary
monitor with a flashing red band after the game closed. Desktop captures did
not contain the band, Windows logged no display-driver TDR and the standard
Windows graphics reset cleared it immediately.

Disabling Auto HDR initially produced a clean run, but the symptom later
returned. It also returned after Windows' windowed-game optimizations were
disabled, so neither setting is documented as the fix. Windows recorded
multiple `LiveKernelEvent` `1a8`/`1b8` reports with `WATCHDOG` dumps during the
test period. This is evidence of a Windows/driver display-pipeline timeout, not
proof of a defective GPU and not a game-rendered red frame.

The repeatable passing condition on this system was **G-SYNC/VRR disabled**.
Ten focus transitions stayed clean: five with the original DPI-scaled test
configuration and five with the final physical 3440x1440/60 configuration,
all stable patch modules enabled and **Windowed at native size** selected. This
points to the G-SYNC/VRR presentation path on the tested mixed-refresh setup,
rather than the ultrawide projection hook. It is not a claim that every driver
or display will behave identically.

1. Close MGS4 and its launcher. Do not keep relaunching while the display is
   unstable.
2. Press `Win+Ctrl+Shift+B`. Both monitors can go black briefly and Windows may
   beep while it resets the graphics pipeline.
3. If the symptom remains, reboot Windows before further testing.
4. In **NVIDIA Control Panel > Set up G-SYNC**, disable G-SYNC temporarily and
   restart the game. The passing test used G-SYNC fully off. NVIDIA also
   documents **Enable for full screen mode** as the lower-impact option when a
   system has difficulty with G-SYNC in windowed applications, but that variant
   was not validated here:
   https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/Display/To_use_variable_refresh_rates.htm
5. Reopen the configurator, choose **Windowed at native size (recommended)**,
   save, and only then attempt another launch. Disabling the Unity bypass alone
   does not change the game's presentation mode.
6. Auto HDR can be disabled separately for troubleshooting, but it was not
   sufficient in this test. Microsoft's documentation explains that windowed
   optimizations, Auto HDR and VRR share the newer flip-model presentation path:
   https://support.microsoft.com/en-us/windows/hardware/display-graphics/optimizations-for-windowed-games-in-windows-11
7. Do not disable SmartScreen, antivirus, driver security or other Windows
   protections. MGS4 Ultra120 never changes NVIDIA or Windows display settings.

Report the GPU, driver version, monitor resolutions/scaling, HDR/G-SYNC state
and whether the symptom also occurs after uninstalling the patch. A desktop
screenshot may not capture a scan-out/VRR fault, so a phone video is useful.

The ASI release completed 20 confirmed automated focus changes and the user's
subsequent manual focus test without an apparent recurrence. G-SYNC was still
disabled during the automated run. Treat this as an acceptance result, not a
universal guarantee for every driver/display pipeline.

## `0xc0000005` immediately after hook installation

Alpha.3 fixes the native execute-protection race found at projection RVA
`0x0e3410`. If a newer build still produces a dump, attach only the textual
`.log`, patch log and executable SHA-256 to an issue. Do not upload proprietary
game binaries or memory dumps publicly.
