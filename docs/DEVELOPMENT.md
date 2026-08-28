# Development

## Requirements

- CMake 3.21 or newer
- A 64-bit Windows C++ compiler, or MinGW-w64 for cross-compiling
- Git/network access for the pinned MinHook source

Native Windows build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Linux cross-build:

```bash
cmake -S . -B build-mingw \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw -j
```

Outputs are written under `build*/bin`. MinHook is pinned to commit
`d94c64d32ea37bc4f5ee47d580709f70c6fb6080`.

## Required native-Windows validation

The proxy must contain the complete WinMM export table and native Windows must
execute every protected-code hook. A Proton-only smoke test is insufficient.
After the MSVC build, run:

```powershell
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts\windows\verify-winmm-proxy.ps1 `
  -ProxyDll build\bin\Release\winmm.dll `
  -SmokeTestExe build\bin\Release\winmm_proxy_smoke.exe
```

Then complete the in-game checklist in [WinMM proxy release gate](WINMM_PROXY.md).
That document records the native `waveOutGetDevCapsW` loader failure and the
Windows execute-protection crash that the original CachyOS validation missed.
It also requires checking physical resolution under Windows DPI scaling and
the Auto HDR multi-monitor presentation path. A successful CachyOS/Proton run
does not waive any native-Windows gate.

Do not distribute proprietary executables, game assets, memory dumps, or
captures containing game resources. When adding a game build, record its full
SHA-256, PE timestamp/image size, expected bytes, and test both renderers.

## Release packages

After a clean successful build, create the platform packages from the repo
root with:

```bash
./scripts/package-release.sh v0.3.1-alpha.3 windows
```

To package MSVC output from Git Bash instead of the default MinGW directory:

```bash
MGS4ULTRA120_BIN_DIR="$PWD/build/bin/Release" \
  ./scripts/package-release.sh v0.3.1-alpha.3 windows
```

The script refuses to package a missing DLL or direct-launch wrapper, copies
only redistributable project files, and emits a SHA-256 checksum beside each
selected platform archive. Use `linux` for a Linux-only package or `all` when a
release intentionally contains both platform lines. Alpha.3 is packaged as a
Windows-only repair; Linux users remain on the prior validated line. GitHub's
automatically generated source archives remain the canonical source packages.
