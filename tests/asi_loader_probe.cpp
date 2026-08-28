#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>

using TimeGetTimeFn = DWORD (WINAPI*)();

int main() {
    HMODULE winmm = LoadLibraryW(L"winmm.dll");
    if (!winmm) {
        std::fprintf(stderr, "Could not load local winmm.dll: %lu\n",
                     GetLastError());
        return 1;
    }
    auto time_get_time = reinterpret_cast<TimeGetTimeFn>(
        GetProcAddress(winmm, "timeGetTime"));
    if (!time_get_time) {
        std::fprintf(stderr, "timeGetTime export is missing: %lu\n",
                     GetLastError());
        return 1;
    }
    const DWORD first = time_get_time();
    Sleep(20);
    const DWORD second = time_get_time();
    if (second < first) {
        std::fputs("timeGetTime moved backwards\n", stderr);
        return 1;
    }
    // Leave the loader resident until process exit so its ASI worker can
    // complete without racing a FreeLibrary call.
    Sleep(1500);
    std::puts("Ultimate ASI Loader proxy probe completed.");
    return 0;
}
