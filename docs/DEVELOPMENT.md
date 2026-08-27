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

Do not distribute proprietary executables, game assets, memory dumps, or
captures containing game resources. When adding a game build, record its full
SHA-256, PE timestamp/image size, expected bytes, and test both renderers.

## Release packages

After a clean successful build, create the platform packages from the repo
root with:

```bash
./scripts/package-release.sh v0.2.1-alpha.1
```

The script refuses to package a missing DLL, copies only redistributable
project files, and emits SHA-256 checksums beside the Windows ZIP and Linux
tarball. GitHub's automatically generated source archives remain the canonical
source packages for a tagged release.
