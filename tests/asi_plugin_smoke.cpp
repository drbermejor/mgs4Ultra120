#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::fputs("usage: asi_plugin_smoke <MGS4Ultra120.asi>\n", stderr);
        return 2;
    }
    HMODULE plugin = LoadLibraryW(argv[1]);
    if (!plugin) {
        std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    // The worker rejects this unsupported probe executable without applying
    // game offsets. Let it complete before unloading the module.
    Sleep(1000);
    if (!FreeLibrary(plugin)) {
        std::fprintf(stderr, "FreeLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    std::puts("ASI plugin load smoke test passed.");
    return 0;
}
