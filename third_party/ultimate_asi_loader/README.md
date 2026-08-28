# Ultimate ASI Loader distribution metadata

The Windows ASI preview uses Ultimate ASI Loader `v9.7.4` by ThirteenAG as its
independent x64 WinMM proxy. The upstream executable is not committed to this
repository. `scripts/windows/fetch-ultimate-asi-loader.ps1` downloads the
official `Ultimate-ASI-Loader-NoPDB_x64.zip`, verifies both the archive and DLL
SHA-256 values, and renames the upstream `dinput8.dll` build to `winmm.dll` as
supported by the upstream project.

- Release: https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.4
- Archive SHA-256: `e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1`
- Extracted DLL SHA-256: `031a3e5576d91dce1e438d36b9a3d462c7334ab4791990a8ff1e3ddc0e132daf`
- Architecture: x86-64
- Upstream file version: `9.7.4`
- License: MIT; see `LICENSE.txt` in this directory.

Release packages include the verified loader binary, the license and this
metadata. Do not update the URL or either hash without repeating the native
Windows loader and migration release gates.
