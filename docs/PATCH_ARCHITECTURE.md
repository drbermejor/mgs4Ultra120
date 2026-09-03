# Patch architecture and update playbook

This document is the maintainer map for the runtime patch. It explains which
game-owned path each module changes, how that path is selected, and what must
be revalidated after an MGS4 update. It complements the user-facing
[technical notes](TECHNICAL.md); it is not a claim that every game build or
screen has been validated.

## Runtime components

```text
mgs4.exe
  -> winmm.dll                         pinned external ASI loader
      -> scripts/MGS4Ultra120.asi      resolution, projection, reticle, input
      -> scripts/MGS4NativeCenteredHUD.asi (optional native UI correction)
      -> scripts/MGSFPSUnlock.asi      optional independent timing patch

Launcher/launcher.exe                  optional reversible direct wrapper
```

Neither project ASI edits `mgs4.exe` on disk. All game changes are in-process
and disappear when the ASIs are removed. The direct wrapper is the exception:
setup backs up the official launcher before replacing it and uninstall restores
that backup only when ownership hashes still match.

## Compatibility and failure model

The public address profile is selected by the executable's PE timestamp and
image size. Its full SHA-256 is recorded in [compatibility](COMPATIBILITY.md).
Protected code is then allowed time to decrypt, and hook sites must match their
expected instructions before MinHook is invoked.

- `MGS4Ultra120.asi` blocks an unknown PE profile by default. The explicit
  `AllowUnsupportedExecutable=1` override remains unsafe because known data
  RVAs cannot be identified by a code signature.
- `MGS4NativeCenteredHUD.asi` has no unsupported-build override. It installs
  nothing when the PE profile or mandatory native UI signatures differ.
- The four reticle edits are classified and validated as one set before any
  byte is written. A mixed or unknown state is rejected.
- Related native-HUD hooks are created and enabled as groups. A failed group is
  rolled back rather than exposing only half of a classifier or compositor.
- Runtime object corrections use caller, resource, topology, coordinate-domain
  and state guards. Unknown objects keep the game's original behavior.

The log beside `mgs4.exe` is part of the failure contract. An update that is
blocked safely should produce an explicit unsupported-build or signature error,
not a silent claim of compatibility.

## Core patch inventory

All RVAs below belong only to the executable profile in
[compatibility](COMPATIBILITY.md).

| Area | Native path | Selection and validation | Effect |
| --- | --- | --- | --- |
| Output/internal resolution | getters `0x65c040`, `0x65c030`; setter `0x65f050` | PE gate, exact getter bytes, setter prologue | Keeps the presentation resolution separate from the optional supersampled internal extent and republishes native render-state mirrors on mode changes. |
| Gameplay FOV | camera builder `0x0b9bb0`, owner return `0x0ba3a3` | PE gate, exact builder prologue, caller ownership | Divides the native camera input scale by `FOVMultiplier`; the game remains responsible for projections, combined matrices and frustum planes. |
| Final aspect correction | projection setter `0x0e3410` | PE gate, decrypted prologue, complete perspective-matrix structure and recognized aspect | Rewrites horizontal projection scale for Hor+ output. It is aspect-only when the native FOV hook owns FOV and becomes the automatic FOV fallback if that hook cannot start. |
| Cinematic FOV preview | owner `0x652e00`, shared camera routes | Explicit opt-in, owner prologue, TLS ownership and final-rebuild continuity | Extends only the observed in-engine cinematic owner; unrelated auxiliary cameras keep their original FOV. |
| Controller profile | detected-profile setter `0x750ec0`, connection mask `0x23d2dbc0` | Explicit opt-in, setter prologue, native nonzero connection mask | Latches the last native controller family while connected so a transient keyboard-profile request cannot neutralize controller axes. It does not create or enumerate devices. |
| Aiming reticle | `0xe39816`, `0xe39830`, `0xe398f1`, `0xe3990c` | Exact original/applied byte classification across all four sites | Replaces signed 16-bit coordinate truncation with 32-bit moves on both axes, removing the 4096-pixel internal-extent overflow. |

The raw resolution mirrors written by `apply_resolution_state()` are
`0x1b00000`, `0x22a8d40`, `0x22a8d48`, `0x1ddda94`, `0x1ddda98`,
`0x1dddaac`, `0x1dddab0`, and `0x3bd1158` through `0x3bd1178`. Adjacent
width/height pairs are published with an atomic 64-bit exchange where their
layout permits it.

## Native centered-HUD inventory

The HUD module works before DX11 or DX12 receives UI geometry. The ordinary
policy maps the native 1280x720 logical canvas into a centered 16:9 safe band.
Exceptions exist because some producers bypass that converter or apply its
horizontal factor twice.

| Area | Native path | Guard | Effect and current boundary |
| --- | --- | --- | --- |
| Core HUD/menu layout | layout converter `0x439810` | Mandatory prologue, safe arithmetic | Reimplements the converter from the current call arguments. Full logical roots retain an output-sized physical viewport with an expanded logical X range. |
| Subtitles and movies | physical rectangle emitter `0x0be090` | Exact caller allowlist | Maps output-pixel rectangles into the centered safe band. Each producer is separately configurable. |
| Inventory previews | semantic owner rectangle `0x4da5b0` | Four exact caller RVAs and in-bounds rectangle | Uniformly scales X and Y so preview aspect is preserved. |
| Pause map | command builder `0x4e9d00` | Two caller returns, descriptor, parent identity, callback, command topology, vertices, colors and UVs | Removes the second horizontal contraction from the two verified map vertex batches around their authored center. |
| Realtime Codec feed | auxiliary surface factory `0x4dbd60` | Caller, type, resource and dimensions | Reduces the auxiliary target width once. The surrounding Codec UI is handled by the core canvas. Some live 3D sequences can still appear compressed and require further validation. |
| Mission Briefing | owner `0x0e6cf20`; child surface `0x0e7ce20` | Exact prologues, caller, four-child index domain and consistent rectangle pairs | Maps the known compositor rectangles horizontally as one hook group. Control/ticker text can still overflow and the screen remains experimental. |
| Full-output modal backgrounds | traversal `0x428510`; dispatcher `0x4278b0`; solid handler `0x425520` | Resource/allocation identity, normal traversal, raw ordinal, identity-transform provenance, node fields and exact geometry | Expands only verified output-covering solids. The classifier trio is installed atomically; geometry alone never opts a node in. |

The values that identify map, Codec, briefing and modal objects live in
`src/native_hud/native_hud_signatures.h`. Pure coordinate operations live in
`src/native_hud/native_hud_math.h` and are covered by unit tests. Keeping those
files separate prevents runtime hook code from becoming the only description
of the model.

## Source ownership by file

- `src/mgs4_ultrawide.cpp`: runtime orchestration and game hooks for the core
  ASI.
- `src/projection_math.h`: pure projection classification and transformation.
- `src/camera_route_policy.h`: exact callers allowed to own gameplay or
  cinematic FOV.
- `src/supersampling_math.h`: checked output-to-internal extent calculation.
- `src/reticle_truncation_patch.h`: complete four-site byte patch definition
  and state classifier.
- `src/native_hud/mgs4_native_centered_hud.cpp`: guarded native UI hooks.
- `src/native_hud/native_hud_math.h`: pure safe-canvas geometry.
- `src/native_hud/native_hud_signatures.h`: semantic identities used by HUD
  exceptions.
- `src/winmm_proxy.cpp`: legacy full WinMM forwarder retained as a regression
  and reproducibility target; public packages use the pinned external loader.
- `launcher_wrapper/mgs4_direct_launcher.cpp`: reversible replacement for the
  Unity launcher, reproducing its observed child-process protocol.

## Tests and what they establish

| Test | Contract |
| --- | --- |
| `projection_math_test` | Perspective recognition, aspect/FOV ownership, close-up acceptance and no-op cases. |
| `supersampling_math_test` | Checked scaling, rounding and rejection of malformed or overflowing extents. |
| `reticle_truncation_patch_test` | Original/applied/mixed/unknown classification of the complete byte set. |
| `native_hud_math_test` | Safe-canvas transforms and exact semantic classifiers that can be tested without the game. |
| `asi_plugin_smoke` | Both project ASIs load as plugins and expose a valid module. |
| `launcher_wrapper_smoke` | Direct wrapper command-line construction, working directory and exit-code forwarding. |
| `winmm_proxy_smoke` | Legacy proxy export forwarding on native Windows. |
| Linux script smoke tests | Steam launch-option and custom-library handling. |

These tests do not prove visual correctness. A release still requires the
native-Windows and in-game checks in [development](DEVELOPMENT.md), including
DX11 and DX12.

## Adapting to a game update

1. **Preserve the previous reference.** Record the last working tag, package
   hashes and visual evidence. Work on a branch or private investigation tree;
   do not weaken the public executable gate to make an unknown build run.
2. **Identify the new executable.** Record SHA-256, PE timestamp and image size.
   Launch once with the old ASIs and confirm that both logs fail closed.
3. **Relocate behavior, not just addresses.** For every row in the inventories,
   find the corresponding function from its old bytes, callers, constants and
   data flow. A matching short byte sequence alone is insufficient.
4. **Build a new profile.** Update PE identity, RVAs and expected bytes together.
   Keep semantic object signatures separate from code prologues. Do not reuse a
   data RVA until its reads and writes have been confirmed in the new build.
5. **Run pure and loader tests.** A clean configure/build and the complete CTest
   suite must pass before the game is modified.
6. **Validate incrementally in one live session where possible.** Start with all
   optional features disabled, then test resolution/projection, reticle, input
   and native HUD independently. Capture configuration, logs and native-size
   screenshots for every A/B comparison.
7. **Exercise visual boundaries.** At minimum test 16:9 control, 21:9 gameplay,
   supersampling above 4096 internal width, pause map, Codec prerecorded and
   realtime feeds, Mission Briefing, subtitles, movies, load/save confirmation
   and title menus in both renderers.
8. **Package from the reviewed commit.** Verify embedded versions, asset names,
   installer behavior and hashes. Release notes must distinguish verified,
   experimental and known-broken behavior.

For each relocated hook, leave a short source comment answering: what native
behavior it owns, why the selector is unique, what is changed, and what happens
when validation fails. Avoid research transcripts and unexplained numeric
constants in public documentation.

## Moving from fixed RVAs to signatures

Address discovery can make routine code-layout updates cheaper, but it must not
turn an unknown executable into an implicitly supported one. The appropriate
design is a **hybrid resolver**:

```text
known PE/SHA profile
  -> resolve code anchors by signatures
  -> decode their relative references and call sites
  -> validate relationships and semantic invariants
  -> require exactly one complete solution
  -> publish an immutable ResolvedProfile
  -> install all dependent hooks or none
```

The known executable identity remains the release gate. Signatures remove the
assumption that every function stays at the same RVA; they do not prove that a
function with similar bytes still has the same meaning after a game update. A
new executable becomes supported only after runtime validation and addition to
the compatibility table.

### Resolver shape

A small shared library can expose data rather than global constants:

```cpp
struct BytePattern {
    const std::uint8_t* bytes;
    const std::uint8_t* significant; // 0 = wildcard, 1 = compare
    std::size_t size;
};

struct ResolvedProfile {
    std::uintptr_t projection_setter;
    std::uintptr_t camera_builder;
    std::uintptr_t resolution_setter;
    std::uintptr_t render_extent;
    std::array<std::uintptr_t, 4> reticle_truncations;
    // Native-HUD functions, call sites and data references follow.
};
```

The implementation should enumerate PE sections and scan only the expected
section (`.text` for code, never the whole address space). It should return all
matches, not the first match. Zero matches and multiple matches are both hard
failures. The final `ResolvedProfile` is published only after every address
required by an enabled feature has passed validation.

### What makes a useful signature

- Include the distinctive operation and nearby control/data flow, not a common
  function prologue alone.
- Wildcard relocation-dependent immediates, call displacements and RIP-relative
  offsets. Decode those operands after matching instead of copying old values.
- Validate instruction boundaries with a decoder before using a match as a hook
  target or patch site.
- For the reticle, match the conversion/division sequence around each lossy
  `movsx`, then verify that the four discovered routes form two X and two Y
  paths before changing any byte.
- For resolution globals, derive the addresses from the getters' RIP-relative
  loads and from setter references. Agreement between independent references is
  stronger than a direct `.data` byte search.
- For FOV ownership, locate both the camera builder and its owning call site.
  Store the resolved return address, rather than comparing `_ReturnAddress()`
  with a hard-coded RVA.
- For native HUD caller allowlists, resolve call sites from signatures of their
  owning functions. The semantic resource/allocation/topology guards remain in
  place because a caller address alone is not an object identity.
- Keep expected replacement-site instructions beside each signature and check
  them again immediately before writing. Discovery and mutation are separate
  phases.

Steam's protected executable adds one constraint: code is not necessarily
searchable when the ASI first loads. The resolver should use the current
bounded decrypt wait, but test a complete signature rather than only a short
prologue. It scans once during initialization and then exits; no signature scan
belongs in a frame-time callback.

### Safe migration order

1. Introduce and unit-test `BytePattern`, PE-section enumeration, unique-match
   handling and relative-operand decoding without changing runtime behavior.
2. Resolve the projection, camera, controller and resolution functions while
   retaining the current RVAs as a test oracle for the supported build. The two
   methods must produce identical addresses.
3. Derive render-size globals and controller state from decoded code references.
   Do not remove a fixed data RVA until at least two native references agree.
4. Convert the reticle as a four-site resolved set and preserve its existing
   original/applied/mixed state classifier.
5. Move native-HUD function targets, then their caller-return selectors. Keep
   object-level semantic guards unchanged.
6. Remove fixed addresses from hook code only after native Windows DX11/DX12
   and Proton produce the same logs and visual results with both resolvers.

### Tests required before relying on the resolver

- Exact match, wildcard match, no match and duplicate-match fixtures.
- Decoy byte sequences that must be rejected by relationship validation.
- Positive and negative RIP-relative/`call rel32` decoding cases.
- A stored, redistributable metadata fixture containing hashes and short
  non-proprietary instruction windows for each supported executable profile.
- A resolver report that lists each symbolic target, match count, decoded RVA
  and validation result without dumping proprietary code or memory.
- An integration test proving that resolution/HUD hook groups perform no writes
  when any required target is unresolved.

This approach survives ordinary address movement and compiler layout changes.
It cannot safely survive a semantic rewrite of the target routine without new
analysis, and the documentation and log should say so explicitly.

## Refactoring backlog

These are maintenance improvements, not claims about the current release.

1. **Centralize executable profiles.** The PE tuple is currently duplicated by
   the core and HUD ASIs. A shared `ExecutableProfile` should own identity,
   RVAs and expected bytes so a game update is one reviewed change set.
   Evolve that structure into the immutable signature-resolved profile above;
   do not scatter pattern scans across individual hook installers.
2. **Make core installation transactional.** The HUD already groups dependent
   hooks. The two resolution getters and related core hooks should gain a
   validate-all/write-all-or-rollback installer and explicit cleanup when hook
   enabling fails.
3. **Separate raw state RVAs from orchestration.** Move resolution mirrors into
   a named profile structure with field-purpose annotations established from
   reads/writes, reducing anonymous constants in `apply_resolution_state()`.
4. **Split the HUD runtime by producer.** Layout, modal, map, Codec and briefing
   hooks can become small translation units sharing one profile and logging
   layer. This will make review ownership and update failures more local.
5. **Expose structured feature status.** Record `disabled`, `unsupported`,
   `signature mismatch`, `installed` or `runtime rejected` per feature in one
   final log summary instead of relying only on chronological messages.
6. **Extract more classifiers into pure code.** Map-stream and Mission Briefing
   validation can be represented as bounded snapshots and unit-tested with
   synthetic fixtures before touching live memory.
7. **Add a release-manifest check.** CI should compare CMake version, `VERSION`,
   package names, embedded file versions and release tag before upload.

Refactoring should not be combined with a new executable-profile port unless a
small change is required for safety. First restore behavior with evidence; then
change structure under the existing test and visual baselines.
