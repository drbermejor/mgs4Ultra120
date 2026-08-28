# Windows troubleshooting

## `waveOutGetDevCapsW` entry-point error

Do not use v0.3.1-alpha.1 or alpha.2 on native Windows. Their two-export WinMM
shim was insufficient for Steam's native client DLL. Install alpha.3 or newer,
whose proxy mirrors the complete 64-bit WinMM export table.

## `Cannot find drive E:` during setup

Alpha.3 and newer skip stale/offline Steam-library entries. If the game itself
is on a disconnected drive, reconnect it or use **Browse** to select the MGS4
folder on an available drive.

## Steam repeats the custom launch-arguments prompt

The original Unity launcher and pre-alpha.3 direct wrapper can both expose the
game tokens on the `mgs4.exe` command line. Some Steam clients then return to
`launcher.exe` after **Continue**, creating an endless confirmation loop.
Alpha.3 enables the direct wrapper by default and passes the tokens only through
the game's official `mgs4_param` bootstrap. Install/update alpha.3, save with
**Skip the Unity launcher** enabled, and launch again. Do not keep approving a
repeating prompt. If it still appears, cancel and attach
`Launcher/mgs4_direct_wrapper.log` to an issue.

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

## `0xc0000005` immediately after hook installation

Alpha.3 fixes the native execute-protection race found at projection RVA
`0x0e3410`. If a newer build still produces a dump, attach only the textual
`.log`, patch log and executable SHA-256 to an issue. Do not upload proprietary
game binaries or memory dumps publicly.
