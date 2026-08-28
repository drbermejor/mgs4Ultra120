# Corrected FPS support on Proton

The Linux package uses the same plugin layout as Windows:

```text
MGS4/winmm.dll                     Ultimate ASI Loader 9.7.4
MGS4/scripts/MGS4Ultra120.asi      rendering/input modules
MGS4/scripts/MGSFPSUnlock.asi      corrected FPS timing
MGS4/scripts/MGSFPSUnlock.ini      30/60/120 target
```

MGSFPSUnlock is an independent project and currently declares no redistribution
license. MGS4 Ultra120 therefore does not include its binary. When corrected
FPS support is selected, the Linux installer downloads version 0.1.0 directly
from its official GitHub release and verifies:

```text
f5dca70b095dd7ea9a6f181677bc37f35a97ffa068ee6fc6b9b269407cde4d8a  MGSFPSUnlock.zip
9da6f4bf1478e78dd94627ef0b1bd8255e0d3cb1cf343464d9951775b0674679  MGSFPSUnlock.asi
```

## Why a local Proton adaptation is required

After MGS4 unpacks its executable code, Wine reports that `.text` mapping as
`PAGE_WRITECOPY` (`0x08`). MinHook normally accepts only `PAGE_EXECUTE_*`
flags and rejects the valid game-code address as non-executable. Native Windows
does not exhibit this reporting difference.

The installer locates one fully pinned MinHook instruction sequence in the
verified official ASI and changes its protection mask from `0xF0` to `0xF8`.
That includes Wine's `PAGE_WRITECOPY` flag. No game executable or game asset is
modified. The resulting local binary must have this SHA-256:

```text
7a52737883dff4cdf641b986d06bf17101c2dd13c3f4862cc633f4d05fb19dc3  MGSFPSUnlock.asi
```

The procedure aborts on any archive, ASI, signature or final-hash mismatch.
The adapted binary is created only on the user's machine and is removed or
restored by the normal uninstaller.

## Runtime validation

The validated environment is GE-Proton10-34, DirectX 12 and the supported Steam
`mgs4.exe`. `logs/MGSFPSUnlock.log` must report all of the following:

- the selected target frame rate;
- spherical-camera normalization;
- character-control timing;
- polygon-demo, wind, SPURS, hair, cloth, physics and ragdoll fixes;
- `All MGS4 hooks installed successfully`.

120 FPS remains experimental at the game level. Scripted sequences and QTEs
still need broader playthrough testing even when every timing hook initializes.
