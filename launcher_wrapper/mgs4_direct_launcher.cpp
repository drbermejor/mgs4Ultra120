#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kMarker[] = L"MGS4ULTRA120_DIRECT_WRAPPER_ACTIVE";

void show_error(const wchar_t* message, DWORD error = 0) {
    std::wstring text(message);
    if (error) {
        text += L"\n\nWindows error: ";
        text += std::to_wstring(error);
    }
    MessageBoxW(nullptr, text.c_str(), L"MGS4 Ultra120 Direct Launcher",
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

bool write_launcher_parameters(const std::wstring& launcher_directory,
                               const std::vector<std::wstring>& arguments) {
    std::string serialized;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (index) serialized.push_back('\x08');
        serialized += utf8(arguments[index]);
    }
    if (serialized.empty() || serialized.size() >= 1024) return false;

    std::array<unsigned char, 1028> output{};
    const std::uint32_t count = static_cast<std::uint32_t>(arguments.size());
    std::memcpy(output.data(), &count, sizeof(count));
    std::memcpy(output.data() + sizeof(count), serialized.data(), serialized.size());

    wchar_t temporary_directory[MAX_PATH] = {};
    const DWORD length = GetTempPathW(MAX_PATH, temporary_directory);
    if (!length || length >= MAX_PATH) return false;
    std::wstring path(temporary_directory, length);
    path += L"mgs4_param";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL okay = WriteFile(file, output.data(), static_cast<DWORD>(output.size()),
                                &written, nullptr);
    if (okay) FlushFileBuffers(file);
    CloseHandle(file);
    return okay && written == output.size();
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
    const std::wstring language = read_token(ini_path, L"Language", L"en");
    const std::wstring controller = read_token(ini_path, L"ControllerType", L"XBOX");
    const std::vector<std::wstring> arguments = {
        L"-region", region,
        L"-lan", language,
        L"-selfregion", self_region,
        L"-resolution", L"0",
        L"-launcherpath", L"launcher.exe",
        L"-ctrltype", controller,
        L"-launcherroot", launcher_directory,
    };
    if (!write_launcher_parameters(launcher_directory, arguments)) {
        show_error(L"Could not write the mgs4_param bootstrap file.", GetLastError());
        return 3;
    }

    std::wstring command_line = L"\"" + game_executable + L"\"";
    for (const std::wstring& argument : arguments)
        command_line += L" \"" + argument + L"\"";
    SetEnvironmentVariableW(kMarker, L"1");

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
    mutable_command.push_back(L'\0');
    const BOOL created = CreateProcessW(game_executable.c_str(), mutable_command.data(),
                                        nullptr, nullptr, FALSE, 0, nullptr,
                                        launcher_directory.c_str(), &startup, &process);
    if (!created) {
        const DWORD error = GetLastError();
        append_log(launcher_directory, "ERROR CreateProcessW=" + std::to_string(error));
        show_error(L"Could not start mgs4.exe.", error);
        return 4;
    }

    append_log(launcher_directory, "Steam-path direct launch; mgs4.exe PID=" +
                                   std::to_string(process.dwProcessId));
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess);
    append_log(launcher_directory, "mgs4.exe exited; code=" +
                                   std::to_string(exit_code));
    return static_cast<int>(exit_code);
}
