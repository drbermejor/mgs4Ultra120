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

## Frame rate ownership

The current Windows MGS4Ultra120 ASI does not write the frame-limit field and
does not register an FPS hotkey. Legacy FPS keys remain in the root INI only for
safe migration and are forced off by setup.

Optional corrected high-frame-rate support is delegated to the independent
`cipherxof/MGSFPSUnlock` ASI. Its MGS4 implementation locates code by validated
signatures and installs separate timing hooks for the target-FPS getter, camera,
character tick, polygon demos, wind, SPURS tasks, cloth, hair/bandana, rigid
body physics and ragdoll contact velocity. Its independent INI contains the
persistent target, normally 120. Guided setup verifies the official 0.1.0
archive and ASI hashes before placing only those two upstream files in
`scripts`.

## Controller profile

The game's detected-profile setter at RVA `0x750ec0` is hooked. Profiles 1-7
are learned only while the game's controller-connected mask at RVA
`0x23d2dbc0` is nonzero. A request for keyboard profile 0 is replaced by the
latched controller profile while connected. When the mask reaches zero the
latch is cleared. All other profile values continue to the original function.

The module works with game-native device detection. It contains no XInput
proxy, controller poller, virtual-device layer or periodic memory writer.

## UI

The D3D12 safe-area prototype is compiled out of release builds. Its shader
classifier also matched some full-screen effects and therefore could not
provide reliable HUD anchoring. The current binary installs no D3D11/D3D12 UI
hooks. Future work requires classifying left, center and right layout elements
before final draw submission.

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

The patch uses a pinned MinHook revision. Disabling ultrawide avoids all project
resolution/projection hooks; disabling the controller fix avoids its profile
hook. The optional MGSFPSUnlock plugin is a separate ASI and has separate
configuration/ownership handling.

The loader, ASI and INI normally survive ordinary Steam verification because
`mgs4.exe` is not edited. Compatibility with a replaced executable is never
assumed; its identity and hook signatures must be analyzed and tested.
