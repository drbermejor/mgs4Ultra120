# Technical notes

## Executable gate

The patch validates the PE timestamp and image size before enabling game
hooks. Unknown builds are blocked unless `AllowUnsupportedExecutable=1`.
Relevant code-hook sites still verify expected bytes after protected code has
initialized. The override cannot validate known data RVAs and therefore remains
unsafe rather than silently claiming compatibility.

## World rendering and FOV

The native camera builder at RVA `0x0b9bb0` is intercepted at its scalar input.
FOV is applied before the game constructs projection matrices, combined
view-projection matrices and visibility planes:

```text
adjusted_camera_scale = original_camera_scale / FOVMultiplier
```

The common renderer projection setter at RVA `0x0e3410` is also intercepted.
With native-camera mode active it changes only `m00` to the configured aspect
and leaves `m11` untouched, preventing a second FOV application. If the native
hook cannot be installed, it automatically falls back to applying both aspect
and FOV in the final setter. Non-perspective, malformed and unknown-aspect
matrices are untouched.
Resolution getters at RVAs `0x65c040` and `0x65c030` return configured internal
dimensions; the central setter at `0x65f050` substitutes them when resolution
state changes. This is event-driven and uses no polling loop.

The withdrawn alpha.6 approach instead modified returned matrices and manually
rebuilt dependent state. That was the source of the tall/thin regression. The
current hook never reconstructs returned camera/frustum structures; it changes
only the original input and lets the game execute its complete native builder.
The final setter also has no old `m00`/`m11` ceiling, so tight close-ups remain
eligible without broad memory scanning or periodic rewriting.

Only the primary caller return RVA `0x0ba3a3` owns native FOV. Routes later in
the same camera rebuild chain receive the already-corrected state and must not
apply the multiplier again. This route isolation fixed WeaponWindow distortion
while retaining gameplay and cinematic FOV.

At 3440x1440, `FOVMultiplier=1.200` is the tested framing recommendation under
this single-owner model. Visual comparison found that `1.000`,
while preserving the original vertical FOV exactly, framed Snake too tightly
in the tested gameplay view. Any value above `1.000` can still show off-camera actors or transitions earlier
than the authored shot intended even though projection and culling are
consistent. Wider values remain useful as development stress tests and are
accepted by the runtime and configurators, but they are untested and remain
the user's responsibility.
The aiming crosshair is confirmed working at 3440x1440 native and at 3440x1440
output with a 5160x2160 internal render after the 16-bit coordinate correction
described below.
The comparison uses the MGS4 entries published through the
[official RPCS3 patch API](https://rpcs3.net/compatibility?patch&api=v1&v=1.2);
PS3 addresses are research references and are not copied into the PC hook.

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

The aiming reticle's X and Y are truncated to signed 16 bits on their way to
the UI canvas. At RVAs `0xe39816` and `0xe3990c`, `movsx edx, cx` keeps only the
low 16 bits of an X position already scaled to 1/16 px:

```text
0xe3980c  cvttss2si ecx, xmm0     ; ecx = screen_x * 16
0xe39816  movsx edx, cx           ; truncates to int16
0xe3981d  lea eax,[rdx+rdx*4]
0xe39820  shl eax, 8              ; x1280, the UI canvas width
0xe39824  idiv [render width]
```

A centred reticle stores `width/2 * 16`, so the value crosses 32767 at exactly
4096 px of internal width (`2048*16 = 32768`). This is the boundary recorded in
v0.3.1-alpha.6 as stable at 3956x1656 and flickering at 4096. At 5120 wide,
`2560*16 = 40960` wraps to -24576 and the reticle is placed at
`-24576*1280/5120 = -6144` in 1/16 canvas units, off the left edge.

Both X sites are replaced with `mov edx, ecx` plus a nop, which keeps the full
32-bit value: `40960*1280/5120 = 10240`, the canvas centre. The matching Y
truncations at RVAs `0xe39830` and `0xe398f1` are removed as well. At the screen
centre, either axis reaches the signed limit at an internal extent of 4096
pixels; Y therefore has more margin in the currently documented 1440p tests,
but retaining it would leave the same latent defect on the other axis.

All four original opcodes are validated before the first byte is changed. The
patch is refused if they are encrypted, unknown or only partly modified, so it
does not intentionally leave a half-applied coordinate conversion.

The correction is resolution-independent and needs no 16-bit coordinate
ceiling. It is applied unconditionally, and each site's bytes are verified
after protected code initializes, so an unrecognized build leaves the
instructions untouched.
The resulting ASI was validated in native Windows gameplay at 3440x1440 output
and 1.50x/5160x2160 internal rendering, where the previously absent reticle was
visible again.

The retired D3D12 safe-area prototype and its draw classifiers have been
removed. `MGS4NativeCenteredHUD.asi` instead hooks the native 1280x720 layout
converter plus guarded surface, preview and modal producers before either
graphics backend receives the UI. This produces a substantially more stable
main HUD while keeping resolution/FOV ownership separate. Exact additional
hooks correct the verified pause-map command stream and apply guarded partial
adjustments to the live Codec auxiliary surface and Mission Briefing compositor
owners. The latter two do not yet guarantee a visually correct result: live
Codec 3D content can remain compressed and briefing text can overflow. Each
path validates its caller, resource, topology and coordinate domain before
writing; unknown states keep the baseline native-layout result.

## Direct launcher

The optional wrapper replaces only `Launcher/launcher.exe` after backing it
up. It derives the adjacent install and game directories and starts
`MGS4/mgs4.exe` with the same application name, child command line and working
directory used by the official Unity launcher. Steam owns its temporary
interception file; the wrapper does not write it. The wrapper waits for the
child and forwards its exit code.

The official launcher's `-resolution 0` token is a resolution slot, not its
window/fullscreen flag. Presentation is stored separately as `WindowMode` in
`launcher_sv`; a traced stock windowed launch retained `-resolution 0`.

## Loader architecture and proxy safeguards

The current alpha.7 release uses the WinMM/ASI separation introduced and
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
