# WinMM proxy release gate

> This is the reproducibility gate for the legacy alpha.3 combined proxy.
> Alpha.4 keeps this build/test target but distributes pinned Ultimate ASI
> Loader plus `MGS4Ultra120.asi`; see the
> [ASI migration record](ASI_MIGRATION.md).

## Windows failure found in v0.3.1-alpha.2

The original proxy exported only `timeBeginPeriod` and `timeGetTime`. That was
enough for `mgs4.exe` under the CachyOS/Proton test path, but not for Windows
native Steam. `steamclient64.dll` imports additional WinMM entry points such as
`waveOutGetDevCapsW`; the Windows loader therefore stopped before game startup
with an entry-point error.

The proxy must mirror the complete 64-bit system WinMM table: ordinals 2-182,
181 exports in total and 180 named exports. `cmake/winmm_exports.txt` is the
reviewed source of truth. `scripts/generate_winmm_proxy.py` creates the `.def`,
resolver table and x64 trampolines for both MSVC/MASM and MinGW/GNU assembly.
Only `timeBeginPeriod` and `timeGetTime` enter patch-specific wrappers; every
other export is forwarded lazily to `%SystemRoot%\System32\winmm.dll`.

## Native page-protection failure

The protected game decrypts hook targets while their page can still report
`PAGE_READWRITE`. The former Linux-derived code temporarily selected
`PAGE_EXECUTE_READWRITE`, installed MinHook and restored that transient
non-executable value. Proton tolerated the timing, but native Windows raised
`0xc0000005` on the first execution at projection RVA `0x0e3410`.

Successful hooks and patched getters now finish as executable code pages. Do
not restore a captured protection unless its base protection already contains
an execute permission. This distinction must remain in future CachyOS work.

## Mandatory checks before a release

On native Windows, from a clean build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts\windows\verify-winmm-proxy.ps1 `
  -ProxyDll build\bin\Release\winmm.dll `
  -SmokeTestExe build\bin\Release\winmm_proxy_smoke.exe
```

Then install on a clean Steam game and check all of the following:

1. No loader entry-point dialog.
2. No new crash dump after the projection, resolution and controller hooks.
3. The log reports corrected projection matrices, not merely installed hooks.
4. The game reaches a menu/load screen at the requested physical resolution.
   At non-100% desktop scaling, verify the INI contains the physical mode (for
   example 3440x1440), not DPI-scaled bounds such as 2752x1152.
5. On mixed-refresh NVIDIA multi-monitor systems, test focus changes with
   G-SYNC/VRR disabled and every stable module enabled. Auto HDR and windowed
   optimizations did not prevent recurrence in the native alpha.3 test. If a
   band appears,
   stop testing and follow `TROUBLESHOOTING_WINDOWS.md` before continuing.
6. Uninstall restores the exact pre-install DLL, INI and Unity launcher.

On CachyOS/MinGW, build the same generated proxy and inspect its PE export
table with `x86_64-w64-mingw32-objdump -p`. A Proton-only launch is not a
replacement for the native Windows export and page-protection tests above.
