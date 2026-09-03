# Third-party notices

MGS4 Ultra 120 downloads and links MinHook at build time. MinHook is Copyright
(c) 2009-2017 Tsuda Kageyu and is distributed under its own 2-clause BSD
license. The exact upstream revision is pinned in `CMakeLists.txt`.

The full MinHook license is available in the upstream project:
https://github.com/TsudaKageyu/minhook/blob/master/LICENSE.txt

The Windows and Linux releases bundle Ultimate ASI Loader `v9.7.4` by
ThirteenAG as the independent x64 `winmm.dll` loader. It is distributed under the MIT
License. The pinned upstream archive/DLL hashes and a copy of its license are
stored under `third_party/ultimate_asi_loader`:
https://github.com/ThirteenAG/Ultimate-ASI-Loader

## MGSFPSUnlock by Cipherxof

The current corrected high-frame-rate implementation is provided entirely by
Cipherxof's independent `MGSFPSUnlock` project. I discarded the original,
limited MGS4 Ultra120 single-value FPS override in favor of Cipherxof's more
comprehensive and specialized MGS4 timing work. MGS4 Ultra120 does not claim
authorship of MGSFPSUnlock or any of its camera, character, animation, cutscene,
physics, cloth, hair, wind, ragdoll or SPURS timing hooks.

- Upstream project: https://github.com/cipherxof/MGSFPSUnlock
- Official version used by Easy Setup: https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0
- Current upstream pre-release for manual Windows installation: https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.3
- Author: https://github.com/cipherxof

Thank you to Cipherxof for publishing this work. Because the upstream repository
currently does not declare a redistribution license, its binary is not bundled
in MGS4 Ultra120 archives. Guided setup downloads the unchanged official ZIP
directly from the upstream release after checking pinned archive and ASI SHA-256
values. Linux additionally creates a locally adapted copy for Wine and verifies
its final hash; that copy is not redistributed.

No game executable, launcher, asset, key, or other copyrighted game file is
included in this repository or in its release packages. METAL GEAR SOLID and
related names are trademarks of their respective owners. This is an
unofficial community project and is not affiliated with or endorsed by
KONAMI.
