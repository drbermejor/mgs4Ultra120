# Signature resolver

## Status

**Every patch address is resolved from signatures. Native Windows validation
passed on the 2026-09-11 Steam executable in both reference and forced
relocation modes, in one renderer. The other renderer (DX11/DX12) and Proton
remain pending.**

## Design

`tools/generate_patch_windows.py` builds `src/patch_windows.h` from an
unpacked copy of the reference executable. Each of the 40 windows is a masked
instruction sequence:

- relative call/jump displacements and RIP-relative displacements are
  wildcards, so code and data movement does not break the signature;
- opcodes, registers, stack offsets and immediates stay significant;
- every window must match exactly once in `.text` (the controller setter body
  is the documented exception: it has a native twin and is selected through
  its caller);
- every named field must decode as the expected `call`, RIP-relative operand
  or patch site, otherwise the generator fails without writing the header.

`src/patch_resolver.h` resolves feature groups from those windows:

| Group | Derived addresses | Cross-checks |
| --- | --- | --- |
| Resolution | getters, setter, render extent, scaled extent, compositor mirror, getter block | startup copy stores the getter results into the render extent; getter data is an adjacent pair; getters exist for every pair of the block; the mirror copy reads the same render extent |
| Camera | builder and three caller routes | each route's call targets the resolved builder |
| Projection, cinematic owner | function entry | unique window |
| Controller | setter and connection mask | setter reached through its caller and matching the body; mask load and store agree |
| Reticle | four truncation sites | all four divisors read the render extent written at startup |
| HUD core, previews, modal, map, Codec, briefing | functions, 13 caller routes, map descriptor and callback | each route calls its resolved function; descriptor and callback are decoded from their native constructor and registration |

Modes:

- **Reference executable** (PE timestamp and image size match): every window
  is required at its recorded RVA. No scan is performed.
- **Unrecognized executable** with `SignatureRelocation=1` (default): every
  window is located by a unique `.text` scan. A group is installed only when
  all of its windows resolve and its cross-checks agree. Scans are spaced
  further apart while protected code is still decrypting.
- `SignatureRelocation=0`: nothing is applied to an unrecognized executable.

Caller RVAs in `camera_route_policy.h`, `native_hud_signatures.h` and the HUD
source remain as reference identities. Resolved return addresses are
translated to those identities, so classifiers and their tests are unchanged.

## Offline verification

```powershell
./build/Release/signature_resolver_test.exe C:\path\to\mgs4.exe.unpacked.exe
```

With the reference image, every group reproduces all 56 reference addresses
both at the recorded RVAs and by unique scan. A corrupted route window fails
the camera group in both modes, and a duplicated signature makes relocation
fail while the reference path still resolves. Without an argument (CI), the
test covers search, masking, anchoring, relative decoding, route verification
and getter decoding on synthetic data.

Cross-version evidence: the generated signatures were compared with every byte
known from the previous 2026-08-25 executable. Ten function windows match that
build completely and five more match on every known prologue byte. Before this
resolver existed, the original five long core signatures had already relocated
correctly from the 2026-08-25 build to the 2026-09-11 build.

## Runtime validation (2026-09-27, native Windows, 3440x1440)

Forced relocation (`SignatureAudit=1`, `ForceSignatureRelocation=1`):

- audit: 15 core and 27 HUD windows with the expected match count, 0 warnings;
- all enabled core groups and all six HUD groups resolved by scan and installed;
- logged resolved RVAs equal the reference profile;
- live Mission Briefing, Codec realtime and pause-map corrections applied;
- HUD scan and audit completed in about 0.33 s.

Reference mode (default configuration):

- `Signature groups: 4 resolved (reference profile addresses); 0 unresolved.`
- all core hooks and all six HUD groups installed; native camera FOV active;
  pause-map correction applied; no warnings.

## Still required

- Repeat both runs with the other renderer and under Proton.
- The first real game update is the decisive test. Keep the log from that
  launch: it names each group that relocated or failed.
- Signatures cannot prove that a changed routine keeps its meaning. Object
  layouts, allocation sizes and resource identities used by the HUD guards are
  not signature-resolved; if they change, those guards reject the object and
  the game keeps its original behavior.

## Regenerating after a game update

1. Unpack the new executable (SteamStub encrypts `.text` on disk).
2. Locate the moved windows (the previous header and the relocation log are
   the starting point), update `WINDOWS` in the generator with the new
   reference RVAs and run it; it refuses non-unique or malformed windows.
3. Update the reference identity in `game_profile.h`, the expected addresses
   in `tests/signature_resolver_test.cpp` and the caller identities.
4. Run the offline test against the new image, then both runtime modes.
