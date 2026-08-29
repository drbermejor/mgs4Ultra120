# Development

## Requirements

- CMake 3.21 or newer
- A 64-bit Windows C++ compiler, or MinGW-w64 for cross-compiling
- Git/network access for the pinned MinHook source
- Inno Setup 6 only when generating the optional setup EXE

Native Windows build:

```powershell
./scripts/windows/fetch-ultimate-asi-loader.ps1
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

Outputs are written under `build*/bin`: the legacy proxy, cross-platform
`MGS4Ultra120.asi`, `MGS4CenteredHUD16x9.asi`, direct `launcher.exe` and test probes. Release packages use
the pinned Ultimate ASI Loader on both Windows and Proton. MinHook is pinned to
commit `d94c64d32ea37bc4f5ee47d580709f70c6fb6080`. Ultimate ASI Loader is pinned
to `v9.7.4`; its fetch script verifies the upstream archive and extracted DLL.

## Required native-Windows validation

The proxy must contain the complete WinMM export table and native Windows must
execute every protected-code hook. A Proton-only smoke test is insufficient.
After the MSVC build, run:

```powershell
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts\windows\verify-winmm-proxy.ps1 `
  -ProxyDll build\bin\Release\winmm.dll `
  -SmokeTestExe build\bin\Release\winmm_proxy_smoke.exe
powershell -NoProfile -File tests\asi_loader_smoke.ps1 `
  -Loader build-third-party\ultimate-asi-loader\winmm.dll `
  -Plugin build\bin\Release\MGS4Ultra120.asi `
  -Probe build\bin\Release\asi_loader_probe.exe
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
./scripts/package-release.sh v0.3.4-alpha.1 all
```

To package MSVC output from Git Bash instead of the default MinGW directory:

```bash
MGS4ULTRA120_BIN_DIR="$PWD/build/bin/Release" \
MGS4ULTRA120_ASI_LOADER="$PWD/build-third-party/ultimate-asi-loader/winmm.dll" \
  ./scripts/package-release.sh v0.3.4-alpha.1 all
```

The script refuses to package a missing loader, ASI or direct-launch wrapper,
copies only redistributable files and creates the ready-to-drag
`Manual-Install` tree. Standalone checksum sidecars are not generated or
uploaded, keeping the public asset list simple. Developers can calculate a
temporary build hash directly when required. GitHub's generated archives remain
the source packages.

To generate the same setup EXE after packaging on Windows:

```powershell
$zip = Resolve-Path .\dist\MGS4Ultra120-v0.3.4-alpha.1-windows-complete.zip
$sha = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash
.\installer\windows\Build-Installer.ps1 `
  -Version v0.3.4-alpha.1 `
  -WindowsZip $zip `
  -ExpectedZipSha256 $sha
```

`Build-Installer.ps1` first runs the complete package smoke test and then calls
Inno Setup. This lets anyone independently build and inspect an installer from
the auditable repository sources and the hash-pinned upstream loader.
