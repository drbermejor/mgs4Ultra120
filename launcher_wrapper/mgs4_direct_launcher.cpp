#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <string>
#include <vector>

#ifndef MGS4ULTRA120_VERSION
#define MGS4ULTRA120_VERSION "development"
#endif
#define MGS4ULTRA120_WIDEN_INNER(value) L##value
#define MGS4ULTRA120_WIDEN(value) MGS4ULTRA120_WIDEN_INNER(value)

namespace {

constexpr wchar_t kMarker[] = L"MGS4ULTRA120_DIRECT_WRAPPER_ACTIVE";
constexpr wchar_t kWindowTitle[] =
    L"MGS4 Ultra120 " MGS4ULTRA120_WIDEN(MGS4ULTRA120_VERSION)
    L" Direct Launcher";

void show_error(const wchar_t* message, DWORD error = 0) {
    std::wstring text(message);
    if (error) {
        text += L"\n\nWindows error: ";
        text += std::to_wstring(error);
    }
    MessageBoxW(nullptr, text.c_str(), kWindowTitle,
                MB_OK | MB_ICONERROR);
}

std::wstring parent_directory(const std::wstring& path) {
    const auto slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash);
}

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()), nullptr, 0,
                                         nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

bool safe_token(const std::wstring& value) {
    if (value.empty() || value.size() > 16) return false;
    for (wchar_t character : value) {
        const bool allowed = (character >= L'a' && character <= L'z') ||
            (character >= L'A' && character <= L'Z') ||
            (character >= L'0' && character <= L'9') || character == L'_' ||
            character == L'-';
        if (!allowed) return false;
    }
    return true;
}

std::wstring read_token(const std::wstring& ini, const wchar_t* key,
                        const wchar_t* fallback) {
    wchar_t buffer[32] = {};
    GetPrivateProfileStringW(L"Launcher", key, fallback, buffer,
                             static_cast<DWORD>(std::size(buffer)), ini.c_str());
    const std::wstring value(buffer);
    return safe_token(value) ? value : std::wstring(fallback);
}

std::wstring normalize_language(std::wstring value) {
    // Alpha.6 exposed "ge" in the configurator, but the game uses "gr".
    if (_wcsicmp(value.c_str(), L"ge") == 0) value = L"gr";
    static constexpr std::array<const wchar_t*, 7> supported = {
        L"en", L"sp", L"fr", L"it", L"gr", L"jp", L"pt"
    };
    for (const wchar_t* candidate : supported) {
        if (_wcsicmp(value.c_str(), candidate) == 0) return candidate;
    }
    return L"en";
}

void append_log(const std::wstring& launcher_directory, const std::string& message) {
    const std::wstring path = launcher_directory + L"\\mgs4_direct_wrapper.log";
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, message.data(), static_cast<DWORD>(message.size()), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
    // mgs4.exe may ask to return to launcher.exe. The original wrapper process
    // is still waiting for the game, so a nested invocation must exit.
    if (GetEnvironmentVariableW(kMarker, nullptr, 0) != 0) return 0;

    wchar_t module_path[MAX_PATH] = {};
    const DWORD module_length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    if (!module_length || module_length >= MAX_PATH) {
        show_error(L"Could not determine the wrapper path.", GetLastError());
        return 1;
    }

    const std::wstring launcher_directory = parent_directory(module_path);
    append_log(launcher_directory,
               "MGS4 Ultra120 " MGS4ULTRA120_VERSION " direct launcher");
    const std::wstring install_directory = parent_directory(launcher_directory);
    const std::wstring game_directory = install_directory + L"\\MGS4";
    const std::wstring game_executable = game_directory + L"\\mgs4.exe";
    const std::wstring ini_path = game_directory + L"\\mgs4_ultrawide.ini";
    if (GetFileAttributesW(game_executable.c_str()) == INVALID_FILE_ATTRIBUTES) {
        show_error(L"MGS4\\mgs4.exe was not found next to the Launcher directory.");
        return 2;
    }

    const std::wstring region = read_token(ini_path, L"Region", L"eu");
    const std::wstring self_region = read_token(ini_path, L"SelfRegion", L"EU");
    const std::wstring language = normalize_language(
        read_token(ini_path, L"Language", L"en"));
    const std::wstring controller = read_token(ini_path, L"ControllerType", L"XBOX");
    // WindowMode lives in the official launcher_sv settings. Native tracing
    // proved that the Unity launcher still emits "-resolution 0" when
    // WindowMode=1 and the game creates its windowed surface. Do not reinterpret
    // this token as a fullscreen boolean: it is an independent resolution slot.
    const std::wstring display_mode = read_token(
        ini_path, L"DisplayMode", L"Fullscreen");
    append_log(launcher_directory,
               "Game language token: " + utf8(language) +
                   "; bootstrap uses the official -lan pair.");
    append_log(launcher_directory,
               _wcsicmp(display_mode.c_str(), L"Windowed") == 0
                   ? "Presentation profile: windowed/native; official resolution slot 0."
                   : "Presentation profile: exclusive; official resolution slot 0.");

    // Match BootGameSteam.StartProcess in the original IL2CPP launcher exactly:
    // lpApplicationName identifies mgs4.exe while lpCommandLine starts with
    // "-region" (there is deliberately no executable/argv[0] prefix). Steam's
    // process interception owns %TEMP%\mgs4_param; that file is not the
    // launcher's option transport and must not be overwritten by this wrapper.
    std::wstring command_line =
        L"-region " + region +
        L" -lan " + language +
        L" -selfregion " + self_region +
        L" -resolution 0"
        L" -launcherpath launcher.exe"
        L" -ctrltype " + controller +
        L" -launcherroot \"" + launcher_directory + L"\"";
    SetEnvironmentVariableW(kMarker, L"1");

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
    mutable_command.push_back(L'\0');
    const BOOL created = CreateProcessW(game_executable.c_str(), mutable_command.data(),
                                        nullptr, nullptr, FALSE, 0, nullptr,
                                        game_directory.c_str(), &startup, &process);
    if (!created) {
        const DWORD error = GetLastError();
        append_log(launcher_directory, "ERROR CreateProcessW=" + std::to_string(error));
        show_error(L"Could not start mgs4.exe.", error);
        return 4;
    }

    append_log(launcher_directory,
               "Original-launcher command-line protocol; language=" + utf8(language) +
                   "; mgs4.exe PID=" + std::to_string(process.dwProcessId));
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess);
    append_log(launcher_directory, "mgs4.exe exited; code=" +
                                   std::to_string(exit_code));
    return static_cast<int>(exit_code);
}
