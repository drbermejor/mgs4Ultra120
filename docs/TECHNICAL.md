# Technical notes

## Executable gate

The patch validates the PE timestamp and image size before enabling game
hooks. Unknown builds are blocked unless `AllowUnsupportedExecutable=1`.
Relevant code-hook sites still verify expected bytes after protected code has
initialized. The override cannot validate known data RVAs and therefore remains
unsafe rather than silently claiming compatibility.

## World rendering

The common engine projection setter at RVA `0x0e3410` is intercepted. For a
positively identified 16:9 or target-aspect perspective matrix:

```text
adjusted_m11 = original_m11 / FOVMultiplier
m00 = sign(m00) * abs(adjusted_m11) / (width / height)
```

Non-perspective and unknown-aspect matrices are untouched. Resolution getters
at RVAs `0x65c040` and `0x65c030` return configured dimensions; the central
setter at `0x65f050` substitutes them when resolution state changes. This is
event-driven and replaces an early diagnostic prototype that rewrote globals
periodically.

## Frame rate and hotkey

The independent FPS module writes the canonical frame-limit field at RVA
`0x1b08df4`. A Windows hotkey message thread uses `RegisterHotKey` and
`WM_HOTKEY` to switch the field once per user press between 60 and 120. There
is no FPS polling loop. This changes the presentation limit only; a separate
script/physics-timestep correction has not been found, which is why 120 remains
experimental.

## Controller profile

The game's detected-profile setter at RVA `0x750ec0` is hooked. Profiles 1-7
are learned only while the game's controller-connected mask at RVA
`0x23d2dbc0` is nonzero. A request for keyboard profile 0 is replaced by the
latched controller profile while connected. When the mask reaches zero the
latch is cleared. All other profile values continue to the original function.

The module works with game-native device detection. It contains no XInput
proxy, controller poller, virtual-device layer or periodic memory writer.

## D3D12 UI prototype

The proxy intercepts `D3D12CreateDevice`, pipeline creation and direct command
lists. A known 948-byte UI DXBC vertex shader is matched by length and its
20-byte DXBC header/hash. Matching draws temporarily receive a centered 16:9
viewport and the original viewport is restored immediately afterwards.

Because some full-screen effects share this shader, it is not a reliable final
draw classifier. The final anchored mode requires interception earlier in the
UI coordinate/layout path. D3D11 UI classification remains unimplemented.

## Direct launcher

The optional wrapper replaces only `Launcher/launcher.exe` after backing it
up. It derives the adjacent install and game directories, serializes launcher
tokens into the temporary `mgs4_param` bootstrap file, starts `MGS4/mgs4.exe`,
waits for the child and forwards its exit code. An inherited marker prevents a
nested return-to-launcher request from creating a loop.

The official launcher's `-resolution 0` token is a resolution slot, not its
window/fullscreen flag. Presentation is stored separately as `WindowMode` in
`launcher_sv`; a traced stock windowed launch retained `-resolution 0`.

## Loader architecture and proxy safeguards

The current alpha.5 release uses the WinMM/ASI separation introduced and
validated in alpha.4:

```text
mgs4.exe -> winmm.dll (Ultimate ASI Loader v9.7.4)
                    -> scripts/MGS4Ultra120.asi (project hooks)
```

Ultimate ASI Loader is fetched from its official release with pinned archive
and extracted-DLL SHA-256 values. The project plugin contains no WinMM exports.
The combined alpha.3 proxy target remains buildable and tested as a legacy
reproducibility gate.

`winmm.dll` mirrors the complete 64-bit system WinMM export table, not only the
two imports visible in `mgs4.exe`. This is required because native Steam's
`steamclient64.dll` also resolves audio/mixer entry points from the local proxy.
The table and forwarding release gate are documented in
[WINMM_PROXY.md](WINMM_PROXY.md).

The protected executable can reveal decrypted signatures while a page still
has a transient non-executable protection. Successful hooks keep that code
executable; restoring the transient value caused a native-Windows execute
violation at projection RVA `0x0e3410` while Proton had tolerated it.

The patch uses a pinned MinHook revision. Each major module has a separate enable flag. Disabling
ultrawide avoids all resolution/projection/UI hooks; disabling FPS avoids the
frame field and hotkey; disabling the controller fix avoids its profile hook.

The loader, ASI and INI normally survive ordinary Steam verification because
`mgs4.exe` is not edited. Compatibility with a replaced executable is never
assumed; its identity and hook signatures must be analyzed and tested.
