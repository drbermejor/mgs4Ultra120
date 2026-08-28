#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fstream>

int wmain(int argc, wchar_t** argv) {
    wchar_t output[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableW(
        L"MGS4ULTRA120_PROBE_OUTPUT", output, MAX_PATH);
    if (!length || length >= MAX_PATH) return 2;
    std::ofstream stream(output, std::ios::binary | std::ios::trunc);
    if (!stream) return 3;
    // The original launcher intentionally supplies an lpCommandLine beginning
    // with "-region", so the CRT exposes that token as argv[0]. Keep it.
    for (int index = 0; index < argc; ++index) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, argv[index], -1,
                                             nullptr, 0, nullptr, nullptr);
        if (size <= 0) return 4;
        std::string value(static_cast<std::size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, argv[index], -1, value.data(), size,
                            nullptr, nullptr);
        value.pop_back();
        stream << value << "\n";
    }
    return stream ? 0 : 5;
}
