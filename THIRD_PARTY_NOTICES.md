# Third-party notices

MGS4 Ultra 120 downloads and links MinHook at build time. MinHook is Copyright
(c) 2009-2017 Tsuda Kageyu and is distributed under its own 2-clause BSD
license. The exact upstream revision is pinned in `CMakeLists.txt`.

The full MinHook license is available in the upstream project:
https://github.com/TsudaKageyu/minhook/blob/master/LICENSE.txt

The Windows release bundles Ultimate ASI Loader `v9.7.4` by ThirteenAG as
the independent x64 `winmm.dll` loader. It is distributed under the MIT
License. The pinned upstream archive/DLL hashes and a copy of its license are
stored under `third_party/ultimate_asi_loader`:
https://github.com/ThirteenAG/Ultimate-ASI-Loader

Corrected high-frame-rate support on Windows is provided by the independent
`cipherxof/MGSFPSUnlock` project, version `0.1.0`:
https://github.com/cipherxof/MGSFPSUnlock

Thank you to cipherxof for the improved MGS4 FPS unlock and its subsystem-level
camera, animation, cutscene and physics timing work. MGS4 Ultra120 does not
claim that work as its own. Because the upstream repository currently does not
declare a redistribution license, its binary is not bundled in our archives.
Guided Windows setup downloads the official upstream ZIP directly and installs
only its ASI and INI after checking pinned archive and ASI SHA-256 values.

No game executable, launcher, asset, key, or other copyrighted game file is
included in this repository or in its release packages. METAL GEAR SOLID and
related names are trademarks of their respective owners. This is an
unofficial community project and is not affiliated with or endorsed by
KONAMI.
