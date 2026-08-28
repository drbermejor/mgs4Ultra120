#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <iterator>

struct WinmmExportSpec {
    std::uint16_t ordinal;
    const char* name;
};

#include "winmm_exports.inc"

static INIT_ONCE g_winmm_once = INIT_ONCE_STATIC_INIT;
static HMODULE g_system_winmm;
static FARPROC g_winmm_functions[std::size(kWinmmExports)]{};

static std::uintptr_t WINAPI unsupported_winmm_export() {
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 1;
}

static BOOL CALLBACK initialize_system_winmm(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t system_directory[MAX_PATH]{};
    const UINT length = GetSystemDirectoryW(system_directory, MAX_PATH);
    if (!length || length + 10 >= MAX_PATH) return FALSE;
    wchar_t path[MAX_PATH]{};
    lstrcpyW(path, system_directory);
    lstrcatW(path, L"\\winmm.dll");
    g_system_winmm = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!g_system_winmm) return FALSE;

    for (std::size_t index = 0; index < std::size(kWinmmExports); ++index) {
        const auto& entry = kWinmmExports[index];
        g_winmm_functions[index] = entry.name
            ? GetProcAddress(g_system_winmm, entry.name)
            : GetProcAddress(g_system_winmm,
                             reinterpret_cast<const char*>(entry.ordinal));
    }
    return TRUE;
}

static bool ensure_system_winmm() {
    return InitOnceExecuteOnce(&g_winmm_once, initialize_system_winmm,
                               nullptr, nullptr) != FALSE && g_system_winmm;
}

extern "C" FARPROC winmm_proxy_resolve(unsigned index) {
    if (ensure_system_winmm() && index < std::size(g_winmm_functions) &&
        g_winmm_functions[index])
        return g_winmm_functions[index];
    return reinterpret_cast<FARPROC>(&unsupported_winmm_export);
}

extern "C" FARPROC winmm_proxy_resolve_by_name(const char* name) {
    if (!name || !ensure_system_winmm()) return nullptr;
    return GetProcAddress(g_system_winmm, name);
}
