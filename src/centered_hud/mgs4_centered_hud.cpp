#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define MMNOTIMER
#include <mmsystem.h>
#include <d3d12.h>
#include <xinput.h>
#undef MMNOTIMER
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cfloat>
#include <intrin.h>
#include <iterator>

#include "MinHook.h"
#include "cache_victim_policy.h"
#include "camera_route_policy.h"
#include "fov_strategy.h"
#include "hud_anchor_math.h"

static HMODULE g_real_winmm;
static float g_target_aspect = 43.0f / 18.0f;
static volatile LONG g_projection_patches;
static volatile LONG g_native_camera_fov_patches;
static volatile LONG g_crosshair_sight_samples;
static volatile LONG g_crosshair_ui_samples;
static std::uintptr_t g_executable_base;
static constexpr std::uintptr_t kSupportedExecutableSize = 0x241be000;
static std::uint32_t g_target_width = 3440;
static std::uint32_t g_target_height = 1440;
static std::uint32_t g_target_fps = 60;
static float g_fov_multiplier = 1.0f;
static bool g_constrain_ui = false;
static bool g_anchor_ui = false;
static bool g_center_hud_16x9 = false;
static bool g_ui_diagnostics = false;
static bool g_cinematic_diagnostics = false;
static bool g_projection_diagnostics = false;
static bool g_crosshair_diagnostics = false;
static bool g_camera_ownership_diagnostics = false;
static bool g_camera_route_test_enabled = false;
static bool g_native_camera_fov_requested = true;
static bool g_native_camera_fov_active = false;
static volatile LONG g_camera_route_test_mode;
static volatile LONG64 g_camera_route_exclusion_mask;
static char g_camera_route_control_path[MAX_PATH];
static std::uint32_t g_input_wakeup_hz;
static bool g_input_diagnostics;
static bool g_lock_controller_input;
static volatile LONG g_locked_controller_profile;
static volatile LONG g_minhook_state;
static volatile LONG g_xinput_calls;
static volatile LONG g_xinput_successes;
static volatile LONG g_xinput_changes;
static volatile LONG g_xinput_packet = -1;
static volatile LONG g_xinput_buttons;
static volatile LONG g_xinput_lx;
static volatile LONG g_pad_update_calls;
static volatile LONG g_pad_convert_calls;
static volatile LONG g_pad_convert_changes;
static volatile LONG g_pad_mapped_buttons;
static volatile LONG g_pad_mapped_lx;
static volatile LONG g_internal_mapped_buttons;
static volatile LONG g_internal_mapped_axes;
static volatile LONG g_pad_publish_calls;
static volatile LONG g_after_publish_buttons;
static volatile LONG g_after_publish_axes;
static volatile LONG g_keyboard_merge_calls;
static volatile LONG g_after_keyboard_pad_buttons;
static volatile LONG g_after_keyboard_pad_axes;
static volatile LONG g_after_keyboard_final_buttons;
static volatile LONG g_after_keyboard_final_axes;
static volatile LONG g_last_input_dispatch_frame = -1;
static volatile LONG g_camera_ownership_samples;
static thread_local unsigned g_cinematic_camera_owner_depth;
static thread_local void* g_cinematic_camera_object;
static thread_local ULONGLONG g_cinematic_camera_tick;
static thread_local float g_cinematic_camera_input_scale;

static constexpr std::uintptr_t kCameraBuilderCallerRvas[] = {
    0x0b9395, 0x0b9ba0, 0x0ba3a3, 0x0b8a43, 0x0b8b4f,
    0x0eb0eb, 0x0eb191, 0x0b97bf, 0x589b1a, 0xab08ed,
    0xab0eb9, 0xe14bcc, 0xa46700, 0xe6bb26, 0xe6bb78,
    0xe6bf70, 0xe6bf93, 0xe6c198, 0x13289dd,
};
static_assert(
    kCameraBuilderCallerRvas[mgs4_fov::kNativeFovPrimaryRouteIndex] == 0x0ba3a3,
    "The native-FOV primary route must remain FUN_1400ba380");

struct alignas(8) CameraOwnershipSlot {
    volatile LONG64 calls;
    volatile LONG64 last_log_ms;
};

// One extra bucket keeps an unexpected caller visible if a future executable
// build adds an indirect or previously unseen route.
static CameraOwnershipSlot
    g_camera_ownership_slots[std::size(kCameraBuilderCallerRvas) + 1];

using TimeBeginPeriodFn = MMRESULT (WINAPI*)(UINT);
using TimeGetTimeFn = DWORD (WINAPI*)();
using SetProjectionFn = void (__fastcall*)(const float* matrix);
using BuildCameraFn = void (__fastcall*)(void* camera, const void* source,
                                         float projection_scale,
                                         float parameter4, float parameter5,
                                         float aspect_scale);
using UpdateCinematicCameraFn = void (__fastcall*)(void* context);
using SetResolutionFn = void (__fastcall*)(std::uint16_t mode, std::int32_t index,
                                           std::uint8_t use_safe_area,
                                           std::uint32_t width,
                                           std::uint32_t height);
using CommonUIFlushFn = void (__fastcall*)(std::uintptr_t, std::uintptr_t*, int);
using GetFPSFn = int (__fastcall*)();
using PlayerSightUpdateFn = std::uint64_t (__fastcall*)(void* sight);
static SetProjectionFn g_original_set_projection;
static BuildCameraFn g_original_build_camera;
static UpdateCinematicCameraFn g_original_update_cinematic_camera;
static SetResolutionFn g_original_set_resolution;
static CommonUIFlushFn g_original_common_ui_flush;
static GetFPSFn g_original_get_fps;
static PlayerSightUpdateFn g_original_player_sight_update;
using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
static XInputGetStateFn g_original_xinput_get_state;
using PadConvertFn = unsigned char (__fastcall*)(void*, const XINPUT_STATE*, void*,
                                                 std::uintptr_t, std::uintptr_t);
using PadUpdateFn = void (__fastcall*)(std::uint32_t, std::intptr_t,
                                       std::uint32_t*, std::uint32_t,
                                       std::int32_t);
static PadConvertFn g_original_pad_convert;
static PadUpdateFn g_original_pad_update;
using VoidInputStageFn = void (__fastcall*)();
static VoidInputStageFn g_original_pad_publish;
static VoidInputStageFn g_original_keyboard_merge;
using SetDetectedProfileFn = void (__fastcall*)(std::int32_t);
static SetDetectedProfileFn g_original_set_detected_profile;

static void apply_resolution_state();
static bool initialize_minhook();
static void log_line(const char* message);

// The public patch is an ASI under MGS4\scripts, while its INI and log live
// beside mgs4.exe. Resolve from the host executable so ASI and proxy loading
// use the same stable paths.
static bool game_sibling_path(char (&path)[MAX_PATH], const char* filename) {
    if (!filename || !GetModuleFileNameA(nullptr, path, MAX_PATH)) return false;
    char* slash = std::strrchr(path, '\\');
    if (!slash) return false;
    const std::size_t prefix = static_cast<std::size_t>(slash + 1 - path);
    const std::size_t length = std::strlen(filename);
    if (prefix + length >= MAX_PATH) return false;
    std::memcpy(path + prefix, filename, length + 1);
    return true;
}
static const char* settings_filename() {
#ifdef MGS4_CENTERED_HUD_LAB
    return "mgs4_centered_hud_16x9.ini";
#elif defined(MGS4_LAB_ONLY)
    return "mgs4_hud_crosshair_test.ini";
#else
    return "mgs4_ultrawide.ini";
#endif
}

static const char* log_filename() {
#ifdef MGS4_CENTERED_HUD_LAB
    return "mgs4_centered_hud_16x9.log";
#elif defined(MGS4_LAB_ONLY)
    return "mgs4_hud_crosshair_test.log";
#else
    return "mgs4_ultrawide.log";
#endif
}

struct CommonUIContext {
    const unsigned char* records;
    std::uint32_t count;
    std::uint32_t cursor;
    LONG batch;
    bool active;
    bool log_batch;
};

struct CommonUISnapshot {
    std::uintptr_t resource;
    std::uint64_t transform_hash;
    std::uint32_t draw[5];
    std::uint32_t record;
    LONG batch;
    bool valid;
};

static thread_local CommonUIContext g_common_ui_context = {};
static volatile LONG g_common_ui_batch_sequence;
static volatile LONG g_cinematic_diagnostic_state = LONG_MIN;

static std::uint64_t fnv1a_bytes(const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::uint64_t hash = 1469598103934665603ull;
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

static CommonUISnapshot take_common_ui_snapshot() {
    CommonUISnapshot snapshot = {};
    CommonUIContext& context = g_common_ui_context;
    if (!context.active || !context.records || context.cursor >= context.count)
        return snapshot;

    const std::uint32_t record_index = context.cursor++;
    const unsigned char* record = context.records +
        static_cast<std::size_t>(record_index) * 0x80u;
    snapshot.transform_hash = fnv1a_bytes(record, 0x60);
    std::memcpy(&snapshot.resource, record + 0x60, sizeof(snapshot.resource));
    std::memcpy(snapshot.draw, record + 0x68, sizeof(snapshot.draw));
    snapshot.record = record_index;
    snapshot.batch = context.batch;
    snapshot.valid = true;
    return snapshot;
}

static void __fastcall hooked_common_ui_flush(std::uintptr_t owner,
                                              std::uintptr_t* backend,
                                              int type) {
    const CommonUIContext previous = g_common_ui_context;
    CommonUIContext current = {};
    if ((g_ui_diagnostics || g_crosshair_diagnostics) && owner && type == 1) {
        current.count = *reinterpret_cast<const std::uint32_t*>(owner + 0x14);
        current.records = *reinterpret_cast<const unsigned char* const*>(owner + 0x20);
        if (current.records && current.count > 0 && current.count <= 100000) {
            current.batch = InterlockedIncrement(&g_common_ui_batch_sequence);
            current.active = true;
            current.log_batch = current.batch <= 128;
            g_common_ui_context = current;

            if (current.log_batch) {
                char message[192] = {};
                std::snprintf(message, sizeof(message),
                    "UIBATCH batch=%ld count=%u records=%p backend=%p",
                    current.batch, current.count,
                    static_cast<const void*>(current.records),
                    static_cast<void*>(backend));
                log_line(message);
            }
        }
    }

    g_original_common_ui_flush(owner, backend, type);

    if (current.active && current.log_batch) {
        char message[160] = {};
        std::snprintf(message, sizeof(message),
            "UIBATCH-END batch=%ld correlated=%u count=%u",
            current.batch, g_common_ui_context.cursor, current.count);
        log_line(message);
    }
    g_common_ui_context = previous;
}

static int __fastcall hooked_get_fps() {
    const int fps = g_original_get_fps();
    bool bink_active = false;
    if (g_executable_base) {
        void* const handle = InterlockedCompareExchangePointer(
            reinterpret_cast<void* volatile*>(g_executable_base + 0x22ecc80),
            nullptr, nullptr);
        bink_active = handle != nullptr;
    }

    const LONG state = (fps & 0xffff) | (bink_active ? 0x10000 : 0);
    const LONG previous = InterlockedExchange(&g_cinematic_diagnostic_state, state);
    if (g_cinematic_diagnostics && previous != state) {
        char message[160] = {};
        std::snprintf(message, sizeof(message),
            "CINEMATIC-DIAG fps=%d bink_active=%s (passive; return unchanged)",
            fps, bink_active ? "yes" : "no");
        log_line(message);
    }
    return fps;
}

static DWORD WINAPI hooked_xinput_get_state(DWORD user_index, XINPUT_STATE* state) {
    InterlockedIncrement(&g_xinput_calls);
    const DWORD result = g_original_xinput_get_state(user_index, state);
    if (result == ERROR_SUCCESS) {
        InterlockedIncrement(&g_xinput_successes);
        if (user_index == 0 && state) {
            const LONG packet = static_cast<LONG>(state->dwPacketNumber);
            if (InterlockedExchange(&g_xinput_packet, packet) != packet)
                InterlockedIncrement(&g_xinput_changes);
            InterlockedExchange(&g_xinput_buttons, state->Gamepad.wButtons);
            InterlockedExchange(&g_xinput_lx, state->Gamepad.sThumbLX);
        }
    }
    return result;
}

static unsigned char __fastcall hooked_pad_convert(void* mapped_state,
                                                   const XINPUT_STATE* xinput_state,
                                                   void* steam_state,
                                                   std::uintptr_t vibration,
                                                   std::uintptr_t history) {
    InterlockedIncrement(&g_pad_convert_calls);
    const unsigned char changed = g_original_pad_convert(
        mapped_state, xinput_state, steam_state, vibration, history);
    if (changed) InterlockedIncrement(&g_pad_convert_changes);
    if (xinput_state) {
        InterlockedExchange(&g_pad_mapped_buttons,
                            static_cast<LONG>(xinput_state->Gamepad.wButtons));
        InterlockedExchange(&g_pad_mapped_lx,
                            static_cast<LONG>(xinput_state->Gamepad.sThumbLX));
    }
    if (mapped_state) {
        InterlockedExchange(&g_internal_mapped_buttons,
                            *reinterpret_cast<volatile LONG*>(mapped_state));
        InterlockedExchange(&g_internal_mapped_axes,
                            *reinterpret_cast<volatile LONG*>(
                                static_cast<unsigned char*>(mapped_state) + 4));
    }
    return changed;
}

static void __fastcall hooked_pad_update(std::uint32_t mask,
                                         std::intptr_t xinput_states,
                                         std::uint32_t* updated_mask,
                                         std::uint32_t flags,
                                         std::int32_t mode) {
    InterlockedIncrement(&g_pad_update_calls);
    g_original_pad_update(mask, xinput_states, updated_mask, flags, mode);
}

static void __fastcall hooked_pad_publish() {
    g_original_pad_publish();
    InterlockedIncrement(&g_pad_publish_calls);
    const auto base = g_executable_base;
    if (!base) return;
    InterlockedExchange(&g_after_publish_buttons,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2af00));
    InterlockedExchange(&g_after_publish_axes,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2af04));
}

static void __fastcall hooked_keyboard_merge() {
    g_original_keyboard_merge();
    InterlockedIncrement(&g_keyboard_merge_calls);
    const auto base = g_executable_base;
    if (!base) return;
    InterlockedExchange(&g_after_keyboard_pad_buttons,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2af00));
    InterlockedExchange(&g_after_keyboard_pad_axes,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2af04));
    InterlockedExchange(&g_after_keyboard_final_buttons,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2ae50));
    InterlockedExchange(&g_after_keyboard_final_axes,
                        *reinterpret_cast<volatile LONG*>(base + 0x23d2ae54));
}

static void __fastcall hooked_set_detected_profile(std::int32_t profile) {
    const auto base = g_executable_base;
    if (g_lock_controller_input && base) {
        const LONG connected_mask =
            *reinterpret_cast<volatile LONG*>(base + 0x23d2dbc0);
        if (!connected_mask) {
            InterlockedExchange(&g_locked_controller_profile, 0);
        } else if (profile >= 1 && profile <= 7) {
            InterlockedExchange(&g_locked_controller_profile, profile);
        } else if (profile == 0) {
            const LONG locked = InterlockedCompareExchange(
                &g_locked_controller_profile, 0, 0);
            if (locked >= 1 && locked <= 7)
                profile = locked;
        }
    }
    g_original_set_detected_profile(profile);
}

struct WindowSearch {
    DWORD process_id;
    HWND best;
    LONG64 area;
};

static BOOL CALLBACK find_process_window(HWND window, LPARAM parameter) {
    auto* search = reinterpret_cast<WindowSearch*>(parameter);
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if (process_id != search->process_id || !IsWindowVisible(window) ||
        GetWindow(window, GW_OWNER) != nullptr)
        return TRUE;
    RECT rectangle{};
    if (!GetWindowRect(window, &rectangle)) return TRUE;
    const LONG64 width = static_cast<LONG64>(rectangle.right) - rectangle.left;
    const LONG64 height = static_cast<LONG64>(rectangle.bottom) - rectangle.top;
    const LONG64 area = width > 0 && height > 0 ? width * height : 0;
    if (area > search->area) {
        search->area = area;
        search->best = window;
    }
    return TRUE;
}

static HWND main_process_window() {
    WindowSearch search{GetCurrentProcessId(), nullptr, 0};
    EnumWindows(find_process_window, reinterpret_cast<LPARAM>(&search));
    return search.best;
}

static DWORD WINAPI input_diagnostic_thread(void*) {
    HWND window = nullptr;
    const DWORD interval = g_input_wakeup_hz
        ? (1000u / (g_input_wakeup_hz > 1000u ? 1000u : g_input_wakeup_hz))
        : 1000u;
    DWORD report_elapsed = 0;
    while (true) {
        if (!window || !IsWindow(window)) window = main_process_window();
        if (g_input_wakeup_hz && window) PostMessageW(window, WM_NULL, 0, 0);
        Sleep(interval);
        report_elapsed += interval;
        if (g_input_diagnostics && report_elapsed >= 1000u) {
            const LONG calls = InterlockedExchange(&g_xinput_calls, 0);
            const LONG successes = InterlockedExchange(&g_xinput_successes, 0);
            const LONG changes = InterlockedExchange(&g_xinput_changes, 0);
            const LONG packet = InterlockedCompareExchange(&g_xinput_packet, 0, 0);
            const LONG buttons = InterlockedCompareExchange(&g_xinput_buttons, 0, 0);
            const LONG lx = InterlockedCompareExchange(&g_xinput_lx, 0, 0);
            const LONG updates = InterlockedExchange(&g_pad_update_calls, 0);
            const LONG converts = InterlockedExchange(&g_pad_convert_calls, 0);
            const LONG converted_changes = InterlockedExchange(&g_pad_convert_changes, 0);
            const LONG mapped_buttons = InterlockedCompareExchange(&g_pad_mapped_buttons, 0, 0);
            const LONG mapped_lx = InterlockedCompareExchange(&g_pad_mapped_lx, 0, 0);
            const LONG internal_buttons = InterlockedCompareExchange(
                &g_internal_mapped_buttons, 0, 0);
            const LONG internal_axes = InterlockedCompareExchange(
                &g_internal_mapped_axes, 0, 0);
            const LONG publishes = InterlockedExchange(&g_pad_publish_calls, 0);
            const LONG after_publish_buttons = InterlockedCompareExchange(
                &g_after_publish_buttons, 0, 0);
            const LONG after_publish_axes = InterlockedCompareExchange(
                &g_after_publish_axes, 0, 0);
            const LONG keyboard_merges = InterlockedExchange(
                &g_keyboard_merge_calls, 0);
            const LONG after_keyboard_pad_buttons = InterlockedCompareExchange(
                &g_after_keyboard_pad_buttons, 0, 0);
            const LONG after_keyboard_pad_axes = InterlockedCompareExchange(
                &g_after_keyboard_pad_axes, 0, 0);
            const LONG after_keyboard_final_buttons = InterlockedCompareExchange(
                &g_after_keyboard_final_buttons, 0, 0);
            const LONG after_keyboard_final_axes = InterlockedCompareExchange(
                &g_after_keyboard_final_axes, 0, 0);
            const auto base = g_executable_base;
            const LONG switch_mode = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c53ed0) : -1;
            const LONG requested_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c53ed4) : -1;
            const LONG detected_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c54504) : -1;
            const LONG selected_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c54508) : -1;
            const LONG cached_detected_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c544f8) : -1;
            const LONG fallback_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c544fc) : -1;
            const LONG cached_selected_profile = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x1c54500) : -1;
            const LONG connected_mask = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2dbc0) : -1;
            const LONG active_slot = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2ae18) : -1;
            const LONG slot_type = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2ae28) : -1;
            const LONG dispatch_frame = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23cb7ab0) : -1;
            const LONG prior_dispatch_frame = InterlockedExchange(
                &g_last_input_dispatch_frame, dispatch_frame);
            const LONG dispatches = prior_dispatch_frame < 0
                ? 0 : dispatch_frame - prior_dispatch_frame;
            const LONG queued_pad_samples = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2f270) : -1;
            const LONG published_pad_buttons = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2af00) : -1;
            const LONG published_pad_axes = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2af04) : -1;
            const LONG final_input_buttons = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2ae50) : -1;
            const LONG final_input_axes = base
                ? *reinterpret_cast<volatile LONG*>(base + 0x23d2ae54) : -1;
            char message[1024] = {};
            std::snprintf(message, sizeof(message),
                          "Input diagnostic: XInput calls/s=%ld success/s=%ld changes/s=%ld packet=%ld buttons=0x%04lx LX=%ld; pad updates/s=%ld converts/s=%ld changed/s=%ld raw=0x%04lx/%ld mapped=0x%08lx/0x%08lx; publish/s=%ld after=0x%08lx/0x%08lx; keyboard-merge/s=%ld after-pad=0x%08lx/0x%08lx after-final=0x%08lx/0x%08lx; dispatch/s=%ld frame=%ld queued=%ld; sampled pad=0x%08lx/0x%08lx final=0x%08lx/0x%08lx; profiles mode=%ld requested=%ld pending=%ld selected=%ld cached=%ld fallback=%ld cached-selected=%ld; connected=0x%lx active=%ld type=%ld; window=%p wakeup=%uHz.",
                          calls, successes, changes, packet, buttons, lx,
                          updates, converts, converted_changes, mapped_buttons,
                          mapped_lx, internal_buttons, internal_axes,
                          publishes, after_publish_buttons, after_publish_axes,
                          keyboard_merges, after_keyboard_pad_buttons,
                          after_keyboard_pad_axes, after_keyboard_final_buttons,
                          after_keyboard_final_axes, dispatches, dispatch_frame,
                          queued_pad_samples, published_pad_buttons,
                          published_pad_axes, final_input_buttons,
                          final_input_axes, switch_mode, requested_profile,
                          detected_profile, selected_profile,
                          cached_detected_profile, fallback_profile,
                          cached_selected_profile, connected_mask,
                          active_slot, slot_type, static_cast<void*>(window),
                          g_input_wakeup_hz);
            log_line(message);
            report_elapsed = 0;
        }
        if (!g_input_wakeup_hz && !g_input_diagnostics) break;
    }
    return 0;
}

static void log_line(const char* message) {
    char module_path[MAX_PATH] = {};
    if (!game_sibling_path(module_path, log_filename())) return;
    HANDLE file = CreateFileA(module_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, message, static_cast<DWORD>(std::strlen(message)), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

static mgs4_fov::CameraRouteTestMode current_camera_route_test_mode() {
    const LONG raw = InterlockedCompareExchange(
        &g_camera_route_test_mode, 0, 0);
    if (raw == static_cast<LONG>(
                   mgs4_fov::CameraRouteTestMode::SelectiveExclusion)) {
        return mgs4_fov::CameraRouteTestMode::SelectiveExclusion;
    }
    if (raw == static_cast<LONG>(
                   mgs4_fov::CameraRouteTestMode::RendererFallback)) {
        return mgs4_fov::CameraRouteTestMode::RendererFallback;
    }
    return mgs4_fov::CameraRouteTestMode::NativeAll;
}

static const char* camera_route_mode_name(
    mgs4_fov::CameraRouteTestMode mode) {
    switch (mode) {
        case mgs4_fov::CameraRouteTestMode::SelectiveExclusion:
            return "selective-exclusion";
        case mgs4_fov::CameraRouteTestMode::RendererFallback:
            return "renderer-fallback";
        default:
            return "native-all";
    }
}

static DWORD WINAPI camera_route_control_thread(void*) {
    constexpr std::uint64_t known_route_mask =
        (std::uint64_t{1} << std::size(kCameraBuilderCallerRvas)) - 1;
    LONG previous_mode = -1;
    LONG64 previous_mask = -1;
    for (;;) {
        const LONG mode = GetPrivateProfileIntA(
            "RouteTest", "Mode", 0, g_camera_route_control_path);
        char mask_text[40] = {};
        GetPrivateProfileStringA("RouteTest", "ExcludedMask", "0",
                                 mask_text, sizeof(mask_text),
                                 g_camera_route_control_path);
        char* end = nullptr;
        const auto parsed = std::strtoull(mask_text, &end, 0);
        const bool valid_mask = end != mask_text && end && *end == '\0';
        const LONG normalized_mode = mode >= 0 && mode <= 2 ? mode : 0;
        const LONG64 normalized_mask = static_cast<LONG64>(
            (valid_mask ? parsed : 0) & known_route_mask);
        InterlockedExchange(&g_camera_route_test_mode, normalized_mode);
        InterlockedExchange64(&g_camera_route_exclusion_mask,
                              normalized_mask);
        if (normalized_mode != previous_mode ||
            normalized_mask != previous_mask) {
            const auto current = current_camera_route_test_mode();
            char message[256] = {};
            std::snprintf(message, sizeof(message),
                "ROUTE-TEST applied: mode=%s excluded_mask=0x%05llx "
                "control=%s",
                camera_route_mode_name(current),
                static_cast<unsigned long long>(normalized_mask),
                g_camera_route_control_path);
            log_line(message);
            previous_mode = normalized_mode;
            previous_mask = normalized_mask;
        }
        Sleep(200);
    }
}

// The game's UI vertex shader is shared by the D3D11 and D3D12 renderers. Its
// DXBC container is stable for the supported executable. Matching the exact
// shader plus reconstructed final bounds provides experimental per-draw UI
// anchoring without changing the compositor's global canvas. Full-screen and
// large effect quads are conservatively left on the original viewport.
static constexpr std::size_t ui_shader_size = 948;
static constexpr unsigned char ui_shader_dxbc_header[20] = {
    0x44, 0x58, 0x42, 0x43, 0xb2, 0xfd, 0xf6, 0xc0, 0x4d, 0x44,
    0xbd, 0x73, 0xc9, 0x70, 0x91, 0x06, 0xa1, 0xc8, 0x5f, 0xa8,
};
static constexpr std::uint64_t ui_image_pixel_shader_hash =
    0x0e4f2118cc17d4eaull;
static constexpr std::uint64_t ui_text_pixel_shader_hash =
    0xb433e6af7598d393ull;

static bool is_ui_shader(const D3D12_SHADER_BYTECODE& shader) {
    return shader.pShaderBytecode && shader.BytecodeLength == ui_shader_size &&
        std::memcmp(shader.pShaderBytecode, ui_shader_dxbc_header,
                    sizeof(ui_shader_dxbc_header)) == 0;
}

static bool is_ui_input_layout(const D3D12_INPUT_LAYOUT_DESC& layout) {
    bool has_uv = false;
    bool has_color = false;
    bool has_position = false;
    if (!layout.pInputElementDescs || layout.NumElements < 3 ||
        layout.NumElements > 16)
        return false;
    for (UINT index = 0; index < layout.NumElements; ++index) {
        const D3D12_INPUT_ELEMENT_DESC& element = layout.pInputElementDescs[index];
        if (!element.SemanticName || element.InputSlot != 0) continue;
        if (std::strcmp(element.SemanticName, "TEXCOORD") == 0 &&
            element.SemanticIndex == 0 &&
            element.Format == DXGI_FORMAT_R16G16_SINT &&
            element.AlignedByteOffset == 0)
            has_uv = true;
        else if (std::strcmp(element.SemanticName, "COLOR") == 0 &&
                 element.SemanticIndex == 0 &&
                 element.Format == DXGI_FORMAT_R8G8B8A8_UNORM &&
                 element.AlignedByteOffset == 4)
            has_color = true;
        else if (std::strcmp(element.SemanticName, "POSITION") == 0 &&
                 element.SemanticIndex == 0 &&
                 element.Format == DXGI_FORMAT_R16G16_SINT &&
                 element.AlignedByteOffset == 8)
            has_position = true;
    }
    return has_uv && has_color && has_position;
}

using D3D12CreateDeviceFn = HRESULT (WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL,
                                               REFIID, void**);
using CreateGraphicsPipelineStateFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
using CreateCommandListFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device*, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator*,
    ID3D12PipelineState*, REFIID, void**);
using CommandListResetFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, ID3D12CommandAllocator*, ID3D12PipelineState*);
using CommandListClearStateFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, ID3D12PipelineState*);
using DrawInstancedFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, UINT, UINT, UINT, UINT);
using DrawIndexedInstancedFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, UINT, UINT, UINT, INT, UINT);
using RSSetViewportsFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, UINT, const D3D12_VIEWPORT*);
using RSSetScissorRectsFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, UINT, const D3D12_RECT*);
using SetPipelineStateFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, ID3D12PipelineState*);
using IASetVertexBuffersFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, UINT, UINT, const D3D12_VERTEX_BUFFER_VIEW*);
using CopyBufferRegionFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, ID3D12Resource*, UINT64, ID3D12Resource*,
    UINT64, UINT64);
using CreateCommittedResourceFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device*, const D3D12_HEAP_PROPERTIES*, D3D12_HEAP_FLAGS,
    const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES,
    const D3D12_CLEAR_VALUE*, REFIID, void**);
using CreatePlacedResourceFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device*, ID3D12Heap*, UINT64, const D3D12_RESOURCE_DESC*,
    D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, REFIID, void**);
using ResourceMapFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Resource*, UINT, const D3D12_RANGE*, void**);
using ResourceUnmapFn = void (STDMETHODCALLTYPE*)(
    ID3D12Resource*, UINT, const D3D12_RANGE*);

static D3D12CreateDeviceFn g_original_d3d12_create_device;
static CreateGraphicsPipelineStateFn g_original_create_graphics_pso;
static CreateCommandListFn g_original_create_command_list;
static CommandListResetFn g_original_command_list_reset;
static CommandListClearStateFn g_original_command_list_clear_state;
static DrawInstancedFn g_original_draw_instanced;
static DrawIndexedInstancedFn g_original_draw_indexed_instanced;
static RSSetViewportsFn g_original_rs_set_viewports;
static RSSetScissorRectsFn g_original_rs_set_scissor_rects;
static SetPipelineStateFn g_original_set_pipeline_state;
static IASetVertexBuffersFn g_original_ia_set_vertex_buffers;
static CopyBufferRegionFn g_original_copy_buffer_region;
static CreateCommittedResourceFn g_original_create_committed_resource;
static CreatePlacedResourceFn g_original_create_placed_resource;
static ResourceMapFn g_original_resource_map;
static ResourceUnmapFn g_original_resource_unmap;
static volatile LONG g_d3d12_device_hooks_installed;
static volatile LONG g_d3d12_command_hooks_installed;
static volatile LONG g_d3d12_export_hook_state;
static volatile LONG g_ui_pipeline_count;
static volatile LONG g_ui_pipeline_ids[128];
static std::uint64_t g_ui_pipeline_ps_hashes[128];
static SIZE_T g_ui_pipeline_ps_sizes[128];
static volatile LONG g_ui_pipeline_exact_vs[128];
static volatile LONG g_ui_pipeline_active[128];
static volatile LONG g_ui_stack_count;
static volatile LONG64 g_ui_stack_keys[256];
static volatile LONG g_d3d12_resource_hooks_installed;

struct ResourceState {
    ID3D12Resource* resource;
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address;
    UINT64 size;
    LONG64 generation;
    volatile LONG64 last_used;
    unsigned char* mapped;
    unsigned char* shadow;
    UINT64 shadow_capacity;
    UINT64 shadow_offset;
    UINT64 shadow_size;
    bool shadow_full_resource;
    bool cpu_accessible;
    volatile LONG ui_relevant;
};

static constexpr std::size_t resource_table_size = 4096;
static constexpr UINT64 max_shadow_copy_size = 2ull * 1024ull * 1024ull;
static constexpr UINT64 max_full_shadow_resource_size = 16ull * 1024ull * 1024ull;
static constexpr UINT64 max_shadow_total_size = 64ull * 1024ull * 1024ull;
static ResourceState g_resources[resource_table_size] = {};
static UINT64 g_shadow_total_size;
static volatile LONG64 g_resource_generation;
static volatile LONG g_resource_table_full_logged;
static volatile LONG g_shadow_budget_logged;
static SRWLOCK g_resource_lock = SRWLOCK_INIT;

static bool create_and_enable_hook(void*, void*, void**, const char*);
static HRESULT STDMETHODCALLTYPE hooked_resource_map(
    ID3D12Resource*, UINT, const D3D12_RANGE*, void**);
static void STDMETHODCALLTYPE hooked_resource_unmap(
    ID3D12Resource*, UINT, const D3D12_RANGE*);

struct CommandListState {
    void* volatile command_list;
    void* volatile pipeline_state;
    D3D12_VIEWPORT viewport;
    volatile LONG has_viewport;
    D3D12_RECT scissor;
    volatile LONG has_scissor;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffers[4];
    volatile LONG vertex_buffer_mask;
};

static constexpr std::size_t state_table_size = 256;
static constexpr std::size_t pipeline_table_size = 128;
static CommandListState g_command_states[state_table_size] = {};
static void* volatile g_ui_pipelines[pipeline_table_size] = {};

static std::size_t pointer_hash(const void* pointer, std::size_t table_size) {
    const auto value = reinterpret_cast<std::uintptr_t>(pointer);
    return static_cast<std::size_t>(((value >> 4) ^ (value >> 17)) % table_size);
}

static LONG64 next_resource_serial() {
    return InterlockedIncrement64(&g_resource_generation);
}

static void discard_shadow_locked(ResourceState* state) {
    if (!state || !state->shadow) return;
    HeapFree(GetProcessHeap(), 0, state->shadow);
    g_shadow_total_size = state->shadow_capacity <= g_shadow_total_size
        ? g_shadow_total_size - state->shadow_capacity
        : 0;
    state->shadow = nullptr;
    state->shadow_capacity = 0;
    state->shadow_offset = 0;
    state->shadow_size = 0;
    state->shadow_full_resource = false;
}

static void initialize_resource_state_locked(ResourceState* state,
                                             ID3D12Resource* resource) {
    if (!state || !resource) return;
    discard_shadow_locked(state);
    *state = {};
    state->resource = resource;
    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    state->size = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
        ? desc.Width : 0;
    state->gpu_address = state->size ? resource->GetGPUVirtualAddress() : 0;
    D3D12_HEAP_PROPERTIES heap = {};
    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
    state->cpu_accessible = SUCCEEDED(resource->GetHeapProperties(
        &heap, &flags)) &&
        (heap.Type == D3D12_HEAP_TYPE_UPLOAD ||
         heap.Type == D3D12_HEAP_TYPE_READBACK ||
         (heap.Type == D3D12_HEAP_TYPE_CUSTOM &&
          heap.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_UNKNOWN &&
          heap.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE));
    state->generation = next_resource_serial();
    InterlockedExchange64(&state->last_used, state->generation);
}

static ResourceState* resource_state_locked(ID3D12Resource* resource,
                                             bool create,
                                             bool newly_created = false) {
    if (!resource) return nullptr;
    std::size_t slot = pointer_hash(resource, resource_table_size);
    ResourceState* oldest = nullptr;
    LONG64 oldest_use = LLONG_MAX;
    bool oldest_ui_relevant = false;
    for (std::size_t probe = 0; probe < resource_table_size; ++probe) {
        ResourceState& state = g_resources[(slot + probe) % resource_table_size];
        if (state.resource == resource) {
            if (create) {
                const D3D12_RESOURCE_DESC desc = resource->GetDesc();
                const UINT64 size = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
                    ? desc.Width : 0;
                const D3D12_GPU_VIRTUAL_ADDRESS address = size
                    ? resource->GetGPUVirtualAddress() : 0;
                // COM wrapper addresses can be reused after destruction. A
                // changed buffer identity invalidates the previous CPU mirror.
                if (newly_created || state.size != size ||
                    state.gpu_address != address)
                    initialize_resource_state_locked(&state, resource);
            }
            InterlockedExchange64(&state.last_used, next_resource_serial());
            return &state;
        }
        if (!state.resource && create) {
            initialize_resource_state_locked(&state, resource);
            return &state;
        }
        if (!state.resource && !create) return nullptr;
        const LONG64 last_use = InterlockedCompareExchange64(
            &state.last_used, 0, 0);
        const bool ui_relevant = InterlockedCompareExchange(
            &state.ui_relevant, 0, 0) != 0;
        if (mgs4_hud::prefer_cache_victim(
                ui_relevant, last_use, oldest != nullptr,
                oldest_ui_relevant, oldest_use)) {
            oldest_use = last_use;
            oldest_ui_relevant = ui_relevant;
            oldest = &state;
        }
    }
    if (create && oldest) {
        if (InterlockedCompareExchange(
                &g_resource_table_full_logged, 1, 0) == 0) {
            log_line("D3D12: resource mirror table reached capacity; recycling least-recently-used entries.");
        }
        initialize_resource_state_locked(oldest, resource);
        return oldest;
    }
    return nullptr;
}

static bool ensure_shadow_capacity_locked(ResourceState* state,
                                          UINT64 required,
                                          bool full_resource) {
    const UINT64 maximum = full_resource
        ? max_full_shadow_resource_size : max_shadow_copy_size;
    if (!state || !required || required > maximum) return false;
    if (state->shadow && state->shadow_capacity >= required &&
        (state->shadow_full_resource || !full_resource))
        return true;

    const auto projected_size = [state, required]() {
        const UINT64 without_current =
            state->shadow_capacity <= g_shadow_total_size
                ? g_shadow_total_size - state->shadow_capacity
                : 0;
        return without_current + required;
    };
    UINT64 projected = projected_size();
    while (projected > max_shadow_total_size) {
        ResourceState* oldest = nullptr;
        LONG64 oldest_use = LLONG_MAX;
        bool oldest_ui_relevant = false;
        for (ResourceState& candidate : g_resources) {
            if (&candidate == state || !candidate.shadow) continue;
            const LONG64 last_use = InterlockedCompareExchange64(
                &candidate.last_used, 0, 0);
            const bool ui_relevant = InterlockedCompareExchange(
                &candidate.ui_relevant, 0, 0) != 0;
            if (mgs4_hud::prefer_cache_victim(
                    ui_relevant, last_use, oldest != nullptr,
                    oldest_ui_relevant, oldest_use)) {
                oldest_use = last_use;
                oldest_ui_relevant = ui_relevant;
                oldest = &candidate;
            }
        }
        if (!oldest) break;
        discard_shadow_locked(oldest);
        projected = projected_size();
        if (InterlockedCompareExchange(&g_shadow_budget_logged, 1, 0) == 0) {
            log_line("D3D12: UI buffer mirror budget reached; recycling least-recently-used mirrors instead of dropping HUD classification.");
        }
    }
    if (projected > max_shadow_total_size) return false;

    unsigned char* replacement = static_cast<unsigned char*>(HeapAlloc(
        GetProcessHeap(), full_resource ? HEAP_ZERO_MEMORY : 0,
        static_cast<SIZE_T>(required)));
    if (!replacement) return false;

    // Promotion from a last-copy slice to a complete resource mirror must
    // retain the bytes which made the buffer recognizable as UI. Future
    // CopyBufferRegion calls then update their real destination offsets.
    if (full_resource && state->shadow && state->shadow_size &&
        mgs4_hud::shadow_range_fits(state->shadow_offset,
                                    state->shadow_size, required)) {
        std::memcpy(replacement + state->shadow_offset, state->shadow,
                    static_cast<std::size_t>(state->shadow_size));
    }
    discard_shadow_locked(state);
    state->shadow = replacement;
    state->shadow_capacity = required;
    state->shadow_offset = full_resource ? 0 : state->shadow_offset;
    state->shadow_size = full_resource ? required : 0;
    state->shadow_full_resource = full_resource;
    g_shadow_total_size += required;
    return true;
}

static const unsigned char* resource_bytes_locked(
    const ResourceState* state, UINT64 begin, UINT64 bytes) {
    if (!state || begin + bytes < begin || begin + bytes > state->size)
        return nullptr;
    if (state->shadow_full_resource && state->shadow &&
        begin + bytes <= state->shadow_capacity)
        return state->shadow + begin;
    if (state->shadow && begin >= state->shadow_offset &&
        begin + bytes <= state->shadow_offset + state->shadow_size) {
        return state->shadow + (begin - state->shadow_offset);
    }
    if (state->mapped)
        return state->mapped + begin;
    return nullptr;
}

static void remember_resource(ID3D12Resource* resource) {
    if (!resource) return;
    // CopyBufferRegion only operates on buffers. Textures do not participate
    // in UI vertex reconstruction and previously exhausted the fixed resource
    // table within minutes on both native D3D12 and Proton.
    if (resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        return;
    AcquireSRWLockExclusive(&g_resource_lock);
    // Creation hooks call this exactly once for the new COM object. Reset an
    // older entry even when the runtime reuses the same wrapper, size and GPU
    // virtual address.
    resource_state_locked(resource, true, true);
    ReleaseSRWLockExclusive(&g_resource_lock);
}

static HRESULT STDMETHODCALLTYPE hooked_resource_map(
    ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* read_range,
    void** data) {
    const HRESULT result = g_original_resource_map(
        resource, subresource, read_range, data);
    if (SUCCEEDED(result) && subresource == 0 && data && *data) {
        AcquireSRWLockExclusive(&g_resource_lock);
        if (ResourceState* state = resource_state_locked(resource, true))
            state->mapped = static_cast<unsigned char*>(*data);
        ReleaseSRWLockExclusive(&g_resource_lock);
    }
    return result;
}

static void STDMETHODCALLTYPE hooked_resource_unmap(
    ID3D12Resource* resource, UINT subresource,
    const D3D12_RANGE* written_range) {
    g_original_resource_unmap(resource, subresource, written_range);
    if (subresource == 0) {
        AcquireSRWLockExclusive(&g_resource_lock);
        if (ResourceState* state = resource_state_locked(resource, false))
            state->mapped = nullptr;
        ReleaseSRWLockExclusive(&g_resource_lock);
    }
}

static void install_resource_hooks(ID3D12Resource* resource) {
    remember_resource(resource);
    if (!resource || InterlockedCompareExchange(&g_d3d12_resource_hooks_installed,
                                                 1, 0) != 0)
        return;
    void** vtable = *reinterpret_cast<void***>(resource);
    const bool map_ok = create_and_enable_hook(
        vtable[8], reinterpret_cast<void*>(&hooked_resource_map),
        reinterpret_cast<void**>(&g_original_resource_map), "D3D12 Resource Map");
    const bool unmap_ok = create_and_enable_hook(
        vtable[9], reinterpret_cast<void*>(&hooked_resource_unmap),
        reinterpret_cast<void**>(&g_original_resource_unmap), "D3D12 Resource Unmap");
    if (!map_ok || !unmap_ok)
        InterlockedExchange(&g_d3d12_resource_hooks_installed, -1);
}

static CommandListState* command_state(ID3D12GraphicsCommandList* command_list,
                                        bool create) {
    std::size_t slot = pointer_hash(command_list, state_table_size);
    for (std::size_t probe = 0; probe < state_table_size; ++probe) {
        CommandListState& state = g_command_states[(slot + probe) % state_table_size];
        void* existing = InterlockedCompareExchangePointer(&state.command_list,
                                                            nullptr, nullptr);
        if (existing == command_list) return &state;
        if (!existing && create &&
            InterlockedCompareExchangePointer(&state.command_list, command_list,
                                              nullptr) == nullptr)
            return &state;
        if (!existing && !create) return nullptr;
    }
    return nullptr;
}

static std::uint64_t shader_bytecode_hash(const D3D12_SHADER_BYTECODE& shader) {
    if (!shader.pShaderBytecode || !shader.BytecodeLength) return 0;
    const auto* bytes = static_cast<const unsigned char*>(shader.pShaderBytecode);
    std::uint64_t hash = 1469598103934665603ull;
    for (SIZE_T index = 0; index < shader.BytecodeLength; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

static void set_ui_pipeline_metadata(
    std::size_t index, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
    bool active) {
    InterlockedExchange(&g_ui_pipeline_active[index], 0);
    InterlockedExchange(&g_ui_pipeline_exact_vs[index],
                        active && desc && is_ui_shader(desc->VS) ? 1 : 0);
    g_ui_pipeline_ps_hashes[index] = active && desc
        ? shader_bytecode_hash(desc->PS) : 0;
    g_ui_pipeline_ps_sizes[index] = active && desc
        ? desc->PS.BytecodeLength : 0;
    if (active) InterlockedExchange(&g_ui_pipeline_active[index], 1);
}

static void invalidate_reused_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return;
    const std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        const std::size_t index = (slot + probe) % pipeline_table_size;
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[index], nullptr, nullptr);
        if (existing == pipeline) {
            set_ui_pipeline_metadata(index, nullptr, false);
            return;
        }
        if (!existing) return;
    }
}

static void remember_ui_pipeline(ID3D12PipelineState* pipeline,
                                 const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) {
    if (!pipeline) return;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        void* volatile* entry = &g_ui_pipelines[(slot + probe) % pipeline_table_size];
        void* existing = InterlockedCompareExchangePointer(entry, nullptr, nullptr);
        if (existing == pipeline) {
            const std::size_t entry_index = (slot + probe) % pipeline_table_size;
            if (InterlockedCompareExchange(
                    &g_ui_pipeline_active[entry_index], 0, 0) == 0) {
                InterlockedExchange(&g_ui_pipeline_ids[entry_index],
                                    InterlockedIncrement(&g_ui_pipeline_count));
            }
            set_ui_pipeline_metadata(entry_index, desc, true);
            return;
        }
        if (!existing && InterlockedCompareExchangePointer(entry, pipeline, nullptr) == nullptr) {
            const LONG count = InterlockedIncrement(&g_ui_pipeline_count);
            const std::size_t entry_index = (slot + probe) % pipeline_table_size;
            InterlockedExchange(&g_ui_pipeline_ids[entry_index], count);
            set_ui_pipeline_metadata(entry_index, desc, true);
            if ((g_ui_diagnostics || g_crosshair_diagnostics) && desc) {
                char message[384] = {};
                std::snprintf(message, sizeof(message),
                    "UIPSO id=%ld ptr=%p vs_exact=%s ps_size=%llu ps_hash=%016llx rts=%u rtv0=%u blend=%u depth=%u topology=%u",
                    count, static_cast<void*>(pipeline),
                    is_ui_shader(desc->VS) ? "yes" : "no",
                    static_cast<unsigned long long>(desc->PS.BytecodeLength),
                    static_cast<unsigned long long>(
                        g_ui_pipeline_ps_hashes[entry_index]),
                    desc->NumRenderTargets,
                    static_cast<unsigned>(desc->RTVFormats[0]),
                    desc->BlendState.RenderTarget[0].BlendEnable ? 1u : 0u,
                    desc->DepthStencilState.DepthEnable ? 1u : 0u,
                    static_cast<unsigned>(desc->PrimitiveTopologyType));
                log_line(message);
            }
            if (count == 1) {
                if (g_center_hud_16x9)
                    log_line("D3D12: exact UI shader recognized; selective centered 16:9 HUD active.");
                else if (g_anchor_ui)
                    log_line("D3D12: UI shader recognized; experimental proportional anchoring active.");
                else if (g_constrain_ui)
                    log_line("D3D12: shared UI/effect shader recognized; experimental centered 16:9 safe area active.");
                else
                    log_line("D3D12: shared UI/effect shader recognized; UI transforms disabled.");
            }
            return;
        }
    }
    log_line("D3D12 ERROR: UI pipeline table is full.");
}

static LONG ui_pipeline_id(ID3D12PipelineState* pipeline) {
    if (!pipeline) return 0;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        const std::size_t index = (slot + probe) % pipeline_table_size;
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[index], nullptr, nullptr);
        if (existing == pipeline)
            return InterlockedCompareExchange(
                       &g_ui_pipeline_active[index], 0, 0) != 0
                ? InterlockedCompareExchange(&g_ui_pipeline_ids[index], 0, 0)
                : 0;
        if (!existing) return 0;
    }
    return 0;
}

static bool is_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return false;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[(slot + probe) % pipeline_table_size], nullptr, nullptr);
        if (existing == pipeline)
            return InterlockedCompareExchange(
                &g_ui_pipeline_active[(slot + probe) % pipeline_table_size],
                0, 0) != 0;
        if (!existing) return false;
    }
    return false;
}

static bool is_exact_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return false;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        const std::size_t index = (slot + probe) % pipeline_table_size;
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[index], nullptr, nullptr);
        if (existing == pipeline)
            return InterlockedCompareExchange(
                       &g_ui_pipeline_active[index], 0, 0) != 0 &&
                InterlockedCompareExchange(
                    &g_ui_pipeline_exact_vs[index], 0, 0) != 0;
        if (!existing) return false;
    }
    return false;
}

static bool is_anchor_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return false;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        const std::size_t index = (slot + probe) % pipeline_table_size;
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[index], nullptr, nullptr);
        if (existing == pipeline) {
            if (InterlockedCompareExchange(
                    &g_ui_pipeline_active[index], 0, 0) == 0)
                return false;
            if (InterlockedCompareExchange(
                    &g_ui_pipeline_exact_vs[index], 0, 0) == 0)
                return false;
            const std::uint64_t pixel_hash = g_ui_pipeline_ps_hashes[index];
            return pixel_hash == ui_image_pixel_shader_hash ||
                pixel_hash == ui_text_pixel_shader_hash;
        }
        if (!existing) return false;
    }
    return false;
}

enum class UIAnchor {
    Unknown,
    FullScreen,
    Left,
    Center,
    Right,
};

static const char* ui_anchor_name(UIAnchor anchor) {
    switch (anchor) {
    case UIAnchor::FullScreen: return "fullscreen";
    case UIAnchor::Left: return "left";
    case UIAnchor::Center: return "center";
    case UIAnchor::Right: return "right";
    default: return "unknown";
    }
}

static UIAnchor classify_ui_draw(CommandListState* state, UINT vertex_count,
                                  UINT first_vertex, float* bounds) {
    if (!state || vertex_count == 0 || vertex_count > 200000 ||
        (InterlockedCompareExchange(&state->vertex_buffer_mask, 0, 0) & 1) == 0)
        return UIAnchor::Unknown;
    const D3D12_VERTEX_BUFFER_VIEW view = state->vertex_buffers[0];
    if (!view.BufferLocation || view.StrideInBytes < 12 ||
        view.StrideInBytes > 256)
        return UIAnchor::Unknown;

    UIAnchor result = UIAnchor::Unknown;
    AcquireSRWLockShared(&g_resource_lock);
    ResourceState* resource = nullptr;
    LONG64 resource_generation = 0;
    for (std::size_t index = 0; index < resource_table_size; ++index) {
        ResourceState& candidate = g_resources[index];
        if (candidate.resource && candidate.gpu_address && candidate.size &&
            view.BufferLocation >= candidate.gpu_address &&
            view.BufferLocation < candidate.gpu_address + candidate.size &&
            candidate.generation > resource_generation) {
            resource = &candidate;
            resource_generation = candidate.generation;
        }
    }
    if (resource) {
        InterlockedExchange(&resource->ui_relevant, 1);
        InterlockedExchange64(&resource->last_used, next_resource_serial());
        const UINT64 begin = view.BufferLocation - resource->gpu_address +
            static_cast<UINT64>(first_vertex) * view.StrideInBytes;
        const UINT64 bytes = static_cast<UINT64>(vertex_count) * view.StrideInBytes;
        const unsigned char* vertices = resource_bytes_locked(
            resource, begin, bytes);
        if (vertices) {
            std::int16_t minimum_x = INT16_MAX;
            std::int16_t minimum_y = INT16_MAX;
            std::int16_t maximum_x = INT16_MIN;
            std::int16_t maximum_y = INT16_MIN;
            for (UINT index = 0; index < vertex_count; ++index) {
                std::int16_t x = 0;
                std::int16_t y = 0;
                const unsigned char* vertex = vertices +
                    static_cast<UINT64>(index) * view.StrideInBytes;
                std::memcpy(&x, vertex + 8, sizeof(x));
                std::memcpy(&y, vertex + 10, sizeof(y));
                if (x < minimum_x) minimum_x = x;
                if (x > maximum_x) maximum_x = x;
                if (y < minimum_y) minimum_y = y;
                if (y > maximum_y) maximum_y = y;
            }
            float virtual_minimum_x = minimum_x / 16.0f;
            float virtual_minimum_y = minimum_y / 16.0f;
            float virtual_maximum_x = maximum_x / 16.0f;
            float virtual_maximum_y = maximum_y / 16.0f;

            // Slot 0 is often local glyph geometry. The exact UI shader also
            // consumes a 96-byte, stride-zero transform from slot 1; without
            // it, top-right labels can look like left-anchored text. Recover
            // the final 1280x720-canvas bounds before choosing an anchor.
            const LONG vertex_mask = InterlockedCompareExchange(
                &state->vertex_buffer_mask, 0, 0);
            if (is_exact_ui_pipeline(reinterpret_cast<ID3D12PipelineState*>(
                    InterlockedCompareExchangePointer(
                        &state->pipeline_state, nullptr, nullptr))) &&
                (vertex_mask & 2) != 0) {
                const D3D12_VERTEX_BUFFER_VIEW transform_view =
                    state->vertex_buffers[1];
                ResourceState* transform_resource = nullptr;
                LONG64 transform_generation = 0;
                for (std::size_t index = 0; index < resource_table_size; ++index) {
                    ResourceState& candidate = g_resources[index];
                    if (candidate.resource && candidate.gpu_address &&
                        candidate.size && transform_view.BufferLocation >=
                            candidate.gpu_address &&
                        transform_view.BufferLocation + 96 >=
                            transform_view.BufferLocation &&
                        transform_view.BufferLocation + 96 <=
                            candidate.gpu_address + candidate.size &&
                        candidate.generation > transform_generation) {
                        transform_resource = &candidate;
                        transform_generation = candidate.generation;
                    }
                }
                if (transform_resource) {
                    InterlockedExchange(&transform_resource->ui_relevant, 1);
                    InterlockedExchange64(&transform_resource->last_used,
                                          next_resource_serial());
                    const UINT64 transform_begin =
                        transform_view.BufferLocation -
                        transform_resource->gpu_address;
                    const unsigned char* transform_bytes =
                        resource_bytes_locked(transform_resource,
                                              transform_begin, 96);
                    if (transform_bytes) {
                        float transform[24] = {};
                        std::memcpy(transform, transform_bytes,
                                    sizeof(transform));
                        float screen_minimum_x = FLT_MAX;
                        float screen_minimum_y = FLT_MAX;
                        float screen_maximum_x = -FLT_MAX;
                        float screen_maximum_y = -FLT_MAX;
                        bool plausible = true;
                        for (UINT index = 0; index < vertex_count; ++index) {
                            std::int16_t x = 0;
                            std::int16_t y = 0;
                            const unsigned char* vertex = vertices +
                                static_cast<UINT64>(index) *
                                    view.StrideInBytes;
                            std::memcpy(&x, vertex + 8, sizeof(x));
                            std::memcpy(&y, vertex + 10, sizeof(y));
                            const float scaled_x = x * transform[0];
                            const float scaled_y = y * transform[1];
                            const float clip_x = scaled_x * transform[12] +
                                scaled_y * transform[16] + transform[20];
                            const float clip_y = scaled_x * transform[13] +
                                scaled_y * transform[17] + transform[21];
                            const float screen_x = (clip_x + 1.0f) * 640.0f;
                            const float screen_y = (1.0f - clip_y) * 360.0f;
                            plausible &= std::isfinite(screen_x) &&
                                std::isfinite(screen_y) &&
                                screen_x >= -4096.0f && screen_x <= 4096.0f &&
                                screen_y >= -4096.0f && screen_y <= 4096.0f;
                            if (screen_x < screen_minimum_x)
                                screen_minimum_x = screen_x;
                            if (screen_x > screen_maximum_x)
                                screen_maximum_x = screen_x;
                            if (screen_y < screen_minimum_y)
                                screen_minimum_y = screen_y;
                            if (screen_y > screen_maximum_y)
                                screen_maximum_y = screen_y;
                        }
                        if (plausible) {
                            virtual_minimum_x = screen_minimum_x;
                            virtual_minimum_y = screen_minimum_y;
                            virtual_maximum_x = screen_maximum_x;
                            virtual_maximum_y = screen_maximum_y;
                        }
                    }
                }
            }
            if (bounds) {
                bounds[0] = virtual_minimum_x;
                bounds[1] = virtual_minimum_y;
                bounds[2] = virtual_maximum_x;
                bounds[3] = virtual_maximum_y;
            }
            const float width = virtual_maximum_x - virtual_minimum_x;
            const float height = virtual_maximum_y - virtual_minimum_y;
            if (width >= 1200.0f && height >= 650.0f) {
                result = UIAnchor::FullScreen;
            } else if ((virtual_minimum_x + virtual_maximum_x) * 0.5f <
                       480.0f) {
                result = UIAnchor::Left;
            } else if ((virtual_minimum_x + virtual_maximum_x) * 0.5f >
                       800.0f) {
                result = UIAnchor::Right;
            } else {
                result = UIAnchor::Center;
            }
        }
    }
    ReleaseSRWLockShared(&g_resource_lock);
    return result;
}

static void log_ui_bounds(ID3D12PipelineState* pipeline, UINT vertex_count,
                          UINT first_vertex, UIAnchor anchor, const float* bounds) {
    if (!g_ui_diagnostics) return;
    static volatile LONG64 keys[512];
    std::uint64_t key = 1469598103934665603ull;
    const int values[] = {static_cast<int>(ui_pipeline_id(pipeline)),
        static_cast<int>(vertex_count),
        static_cast<int>(first_vertex), static_cast<int>(anchor),
        static_cast<int>(bounds[0] * 16.0f), static_cast<int>(bounds[1] * 16.0f),
        static_cast<int>(bounds[2] * 16.0f), static_cast<int>(bounds[3] * 16.0f)};
    for (int value : values) {
        key ^= static_cast<std::uint32_t>(value);
        key *= 1099511628211ull;
    }
    if (!key) key = 1;
    const std::size_t start = static_cast<std::size_t>(key % 512);
    for (std::size_t probe = 0; probe < 512; ++probe) {
        volatile LONG64* slot = &keys[(start + probe) % 512];
        const LONG64 old = InterlockedCompareExchange64(
            slot, static_cast<LONG64>(key), 0);
        if (old == static_cast<LONG64>(key)) return;
        if (old == 0) break;
    }
    static volatile LONG sequence;
    const LONG number = InterlockedIncrement(&sequence);
    if (number > 512) return;
    char message[256] = {};
    std::snprintf(message, sizeof(message),
        "UIBOUNDS #%ld pso=%ld vertices=%u first=%u anchor=%s bounds=%.2f,%.2f..%.2f,%.2f",
        number, ui_pipeline_id(pipeline), vertex_count, first_vertex,
        ui_anchor_name(anchor),
        bounds[0], bounds[1], bounds[2], bounds[3]);
    log_line(message);
}

static void log_ui_correlation(ID3D12PipelineState* pipeline,
                               const char* draw_kind,
                               UINT element_count,
                               const CommonUISnapshot& snapshot) {
    if (!g_ui_diagnostics || !snapshot.valid) return;

    static volatile LONG64 keys[1024];
    std::uint64_t key = 1469598103934665603ull;
    const std::uint64_t values[] = {
        static_cast<std::uint64_t>(ui_pipeline_id(pipeline)),
        static_cast<std::uint64_t>(element_count),
        static_cast<std::uint64_t>(snapshot.resource),
        snapshot.transform_hash,
        snapshot.draw[0], snapshot.draw[1], snapshot.draw[2],
        snapshot.draw[3], snapshot.draw[4],
    };
    for (const std::uint64_t value : values) {
        key ^= value;
        key *= 1099511628211ull;
    }
    if (!key) key = 1;
    const std::size_t start = static_cast<std::size_t>(key % 1024);
    for (std::size_t probe = 0; probe < 1024; ++probe) {
        volatile LONG64* slot = &keys[(start + probe) % 1024];
        const LONG64 old = InterlockedCompareExchange64(
            slot, static_cast<LONG64>(key), 0);
        if (old == static_cast<LONG64>(key)) return;
        if (old == 0) break;
    }

    static volatile LONG sequence;
    const LONG number = InterlockedIncrement(&sequence);
    if (number > 1024) return;
    char message[512] = {};
    std::snprintf(message, sizeof(message),
        "UICORR #%ld batch=%ld record=%u kind=%s pso=%ld elements=%u "
        "resource=%p transform=%016llx draw=%08x,%08x,%08x,%08x,%08x",
        number, snapshot.batch, snapshot.record, draw_kind,
        ui_pipeline_id(pipeline), element_count,
        reinterpret_cast<void*>(snapshot.resource),
        static_cast<unsigned long long>(snapshot.transform_hash),
        snapshot.draw[0], snapshot.draw[1], snapshot.draw[2],
        snapshot.draw[3], snapshot.draw[4]);
    log_line(message);
}

static void log_crosshair_ui_candidate(ID3D12PipelineState* pipeline,
                                       const char* draw_kind,
                                       UINT element_count,
                                       UIAnchor anchor,
                                       const float* bounds,
                                       const CommonUISnapshot& snapshot) {
    if (!g_crosshair_diagnostics || !snapshot.valid) return;

    bool centered_small = false;
    if (bounds && anchor == UIAnchor::Center) {
        const float width = bounds[2] - bounds[0];
        const float height = bounds[3] - bounds[1];
        const float center_x = (bounds[0] + bounds[2]) * 0.5f;
        const float center_y = (bounds[1] + bounds[3]) * 0.5f;
        centered_small = std::isfinite(width) && std::isfinite(height) &&
            width >= 2.0f && width <= 320.0f &&
            height >= 2.0f && height <= 320.0f &&
            center_x >= 480.0f && center_x <= 800.0f &&
            center_y >= 200.0f && center_y <= 520.0f;
    }
    // Indexed common-UI records cannot be reconstructed with the instanced
    // vertex path, so retain them as candidates. Runtime comparison will show
    // whether one resource disappears with the reticle.
    if (!centered_small && std::strcmp(draw_kind, "indexed") != 0) return;

    const LONG sample = InterlockedIncrement(&g_crosshair_ui_samples);
    if (sample > 4096) return;
    char message[512] = {};
    std::snprintf(message, sizeof(message),
        "CROSSHAIR-UI sample=%ld tick=%llu kind=%s pso=%ld elements=%u "
        "resource=%p transform=%016llx anchor=%s "
        "bounds=(%.2f,%.2f..%.2f,%.2f) draw=%08x,%08x,%08x,%08x,%08x",
        sample, static_cast<unsigned long long>(GetTickCount64()), draw_kind,
        ui_pipeline_id(pipeline), element_count,
        reinterpret_cast<void*>(snapshot.resource),
        static_cast<unsigned long long>(snapshot.transform_hash),
        ui_anchor_name(anchor),
        bounds ? bounds[0] : 0.0f, bounds ? bounds[1] : 0.0f,
        bounds ? bounds[2] : 0.0f, bounds ? bounds[3] : 0.0f,
        snapshot.draw[0], snapshot.draw[1], snapshot.draw[2],
        snapshot.draw[3], snapshot.draw[4]);
    log_line(message);
}

static void log_ui_draw_stack(UINT vertex_count, UINT first_vertex,
                              UINT instance_count) {
    if (!g_ui_diagnostics || !g_executable_base) return;

    void* frames[24] = {};
    const USHORT frame_count = RtlCaptureStackBackTrace(
        0, static_cast<ULONG>(sizeof(frames) / sizeof(frames[0])), frames, nullptr);
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_executable_base);
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        g_executable_base + static_cast<std::uintptr_t>(dos->e_lfanew));
    const std::uintptr_t executable_end =
        g_executable_base + nt->OptionalHeader.SizeOfImage;

    std::uintptr_t game_frames[12] = {};
    std::size_t game_count = 0;
    for (USHORT index = 0; index < frame_count && game_count < 12; ++index) {
        const auto address = reinterpret_cast<std::uintptr_t>(frames[index]);
        if (address >= g_executable_base && address < executable_end)
            game_frames[game_count++] = address - g_executable_base;
    }
    if (!game_count) return;

    std::uint64_t key = 1469598103934665603ull;
    key ^= static_cast<std::uint64_t>(vertex_count);
    key *= 1099511628211ull;
    key ^= static_cast<std::uint64_t>(first_vertex);
    key *= 1099511628211ull;
    for (std::size_t index = 0; index < game_count; ++index) {
        key ^= static_cast<std::uint64_t>(game_frames[index]);
        key *= 1099511628211ull;
    }
    if (!key) key = 1;
    const std::size_t start = static_cast<std::size_t>(key % 256);
    for (std::size_t probe = 0; probe < 256; ++probe) {
        volatile LONG64* slot = &g_ui_stack_keys[(start + probe) % 256];
        const LONG64 old = InterlockedCompareExchange64(
            slot, static_cast<LONG64>(key), 0);
        if (old == static_cast<LONG64>(key)) return;
        if (old == 0) break;
    }
    const LONG number = InterlockedIncrement(&g_ui_stack_count);
    if (number > 256) return;

    char message[768] = {};
    int used = std::snprintf(message, sizeof(message),
        "UISTACK #%ld vertices=%u first=%u instances=%u game_rvas=",
        number, vertex_count, first_vertex, instance_count);
    for (std::size_t index = 0; index < game_count && used > 0 &&
         static_cast<std::size_t>(used) < sizeof(message); ++index) {
        used += std::snprintf(message + used, sizeof(message) - used,
                              "%s0x%llx", index ? "," : "",
                              static_cast<unsigned long long>(game_frames[index]));
    }
    log_line(message);

    for (std::size_t frame = 0; frame < game_count && frame < 3; ++frame) {
        const auto address = g_executable_base + game_frames[frame];
        char bytes_message[512] = {};
        int bytes_used = std::snprintf(
            bytes_message, sizeof(bytes_message), "UIBYTES rva=0x%llx data=",
            static_cast<unsigned long long>(game_frames[frame]));
        const auto* bytes = reinterpret_cast<const unsigned char*>(address - 24);
        for (std::size_t index = 0; index < 48 && bytes_used > 0 &&
             static_cast<std::size_t>(bytes_used) < sizeof(bytes_message); ++index) {
            bytes_used += std::snprintf(bytes_message + bytes_used,
                sizeof(bytes_message) - bytes_used, "%02x", bytes[index]);
        }
        log_line(bytes_message);
    }
}

static bool make_ui_viewport(CommandListState* state, D3D12_VIEWPORT* safe,
                             UIAnchor anchor, UINT vertex_count,
                             const float* bounds) {
    if ((!g_constrain_ui && !g_anchor_ui && !g_center_hud_16x9) ||
        !state || !safe ||
        InterlockedCompareExchange(&state->has_viewport, 0, 0) == 0)
        return false;
    auto* pipeline = reinterpret_cast<ID3D12PipelineState*>(
        InterlockedCompareExchangePointer(&state->pipeline_state, nullptr, nullptr));
    if (!is_ui_pipeline(pipeline)) return false;

    const D3D12_VIEWPORT original = state->viewport;

    const bool selective_hud = g_anchor_ui || g_center_hud_16x9;
    if (selective_hud) {
        // Several post-processing pipelines reuse the UI input layout but not
        // the actual UI vertex shader. Restrict proportional anchoring to the
        // exact shader; otherwise those full-screen passes can become a
        // 2560-pixel colour/effect strip on a 3440-pixel output.
        if (!is_anchor_ui_pipeline(pipeline))
            return false;
        if (!bounds || anchor == UIAnchor::Unknown ||
            anchor == UIAnchor::FullScreen || vertex_count == 0 ||
            vertex_count > 1536)
            return false;
        const float width = bounds[2] - bounds[0];
        const float height = bounds[3] - bounds[1];
        if (!std::isfinite(width) || !std::isfinite(height) ||
            width <= 0.0f || height <= 0.0f)
            return false;
        // Single-quad post-processing tiles share the exact UI shader and
        // vertex layout. Exclude them by geometry: ordinary HUD icons/panels
        // are smaller, while long menu bars deliberately remain edge-to-edge.
        if (vertex_count == 6 &&
            (width >= 512.0f || height >= 360.0f ||
             width * height >= 19600.0f))
            return false;
    }
    mgs4_hud::HorizontalAnchor horizontal = mgs4_hud::HorizontalAnchor::Center;
    if (g_anchor_ui && !g_center_hud_16x9 && anchor == UIAnchor::Left)
        horizontal = mgs4_hud::HorizontalAnchor::Left;
    else if (g_anchor_ui && !g_center_hud_16x9 && anchor == UIAnchor::Right)
        horizontal = mgs4_hud::HorizontalAnchor::Right;
    mgs4_hud::SafeViewport calculated{};
    if (!mgs4_hud::make_16x9_safe_viewport(
            original.TopLeftX, original.Width, original.Height, horizontal,
            &calculated))
        return false;
    *safe = original;
    safe->TopLeftX = calculated.left;
    safe->Width = calculated.width;
    return true;
}

static bool make_ui_scissor(CommandListState* state,
                            const D3D12_VIEWPORT& safe_viewport,
                            UIAnchor anchor, D3D12_RECT* safe_scissor) {
    if ((!g_anchor_ui && !g_center_hud_16x9) || !state || !safe_scissor ||
        InterlockedCompareExchange(&state->has_scissor, 0, 0) == 0)
        return false;

    const D3D12_VIEWPORT original = state->viewport;
    if (original.Width <= 0.0f || safe_viewport.Width <= 0.0f)
        return false;

    const float scale = safe_viewport.Width / original.Width;
    float pivot = original.TopLeftX;
    if (g_center_hud_16x9 || anchor == UIAnchor::Center)
        pivot += original.Width * 0.5f;
    else if (anchor == UIAnchor::Right)
        pivot += original.Width;

    const D3D12_RECT source = state->scissor;
    *safe_scissor = source;
    safe_scissor->left = static_cast<LONG>(std::lround(
        pivot + (static_cast<float>(source.left) - pivot) * scale));
    safe_scissor->right = static_cast<LONG>(std::lround(
        pivot + (static_cast<float>(source.right) - pivot) * scale));
    return safe_scissor->right > safe_scissor->left;
}

static HRESULT STDMETHODCALLTYPE hooked_command_list_reset(
    ID3D12GraphicsCommandList* command_list, ID3D12CommandAllocator* allocator,
    ID3D12PipelineState* initial_state) {
    const HRESULT result = g_original_command_list_reset(command_list, allocator,
                                                          initial_state);
    if (SUCCEEDED(result)) {
        if (CommandListState* state = command_state(command_list, true)) {
            InterlockedExchangePointer(&state->pipeline_state, initial_state);
            InterlockedExchange(&state->has_viewport, 0);
            InterlockedExchange(&state->has_scissor, 0);
            InterlockedExchange(&state->vertex_buffer_mask, 0);
        }
    }
    return result;
}

static void STDMETHODCALLTYPE hooked_command_list_clear_state(
    ID3D12GraphicsCommandList* command_list, ID3D12PipelineState* pipeline) {
    if (CommandListState* state = command_state(command_list, true))
        InterlockedExchangePointer(&state->pipeline_state, pipeline);
    g_original_command_list_clear_state(command_list, pipeline);
}

static void STDMETHODCALLTYPE hooked_rs_set_viewports(
    ID3D12GraphicsCommandList* command_list, UINT count,
    const D3D12_VIEWPORT* viewports) {
    if (CommandListState* state = command_state(command_list, true)) {
        if (count && viewports) {
            state->viewport = viewports[0];
            InterlockedExchange(&state->has_viewport, 1);
        } else {
            InterlockedExchange(&state->has_viewport, 0);
        }
    }
    g_original_rs_set_viewports(command_list, count, viewports);
}

static void STDMETHODCALLTYPE hooked_rs_set_scissor_rects(
    ID3D12GraphicsCommandList* command_list, UINT count,
    const D3D12_RECT* scissors) {
    if (CommandListState* state = command_state(command_list, true)) {
        if (count && scissors) {
            state->scissor = scissors[0];
            InterlockedExchange(&state->has_scissor, 1);
        } else {
            InterlockedExchange(&state->has_scissor, 0);
        }
    }
    g_original_rs_set_scissor_rects(command_list, count, scissors);
}

static void STDMETHODCALLTYPE hooked_set_pipeline_state(
    ID3D12GraphicsCommandList* command_list, ID3D12PipelineState* pipeline) {
    if (CommandListState* state = command_state(command_list, true))
        InterlockedExchangePointer(&state->pipeline_state, pipeline);
    g_original_set_pipeline_state(command_list, pipeline);
}

static void STDMETHODCALLTYPE hooked_ia_set_vertex_buffers(
    ID3D12GraphicsCommandList* command_list, UINT start_slot, UINT count,
    const D3D12_VERTEX_BUFFER_VIEW* views) {
    if (CommandListState* state = command_state(command_list, true)) {
        LONG mask = InterlockedCompareExchange(&state->vertex_buffer_mask, 0, 0);
        for (UINT index = 0; index < count && start_slot + index < 4; ++index) {
            const UINT slot = start_slot + index;
            if (views) {
                state->vertex_buffers[slot] = views[index];
                mask |= 1 << slot;
            } else {
                state->vertex_buffers[slot] = {};
                mask &= ~(1 << slot);
            }
        }
        InterlockedExchange(&state->vertex_buffer_mask, mask);
    }
    g_original_ia_set_vertex_buffers(command_list, start_slot, count, views);
}

static void shadow_buffer_copy(ID3D12Resource* destination, UINT64 destination_offset,
                               ID3D12Resource* source, UINT64 source_offset,
                               UINT64 byte_count) {
    if ((!g_anchor_ui && !g_center_hud_16x9 && !g_ui_diagnostics &&
         !g_crosshair_diagnostics) ||
        !destination || !source ||
        !byte_count || byte_count > max_shadow_copy_size)
        return;
    bool has_mapping = false;
    bool can_map = false;
    AcquireSRWLockShared(&g_resource_lock);
    if (ResourceState* state = resource_state_locked(source, false)) {
        has_mapping = state->mapped != nullptr;
        can_map = state->cpu_accessible;
    }
    ReleaseSRWLockShared(&g_resource_lock);

    bool temporary_mapping = false;
    void* temporary_data = nullptr;
    if (!has_mapping && can_map &&
        SUCCEEDED(source->Map(0, nullptr, &temporary_data)) &&
        temporary_data)
        temporary_mapping = true;

    AcquireSRWLockExclusive(&g_resource_lock);
    ResourceState* source_state = resource_state_locked(source, true);
    ResourceState* destination_state = resource_state_locked(destination, true);
    if (source_state && destination_state && source_state->mapped &&
        source_offset + byte_count >= source_offset &&
        source_offset + byte_count <= source_state->size &&
        destination_offset + byte_count >= destination_offset &&
        destination_offset + byte_count <= destination_state->size) {
        InterlockedExchange64(&source_state->last_used, next_resource_serial());
        InterlockedExchange64(&destination_state->last_used,
                              next_resource_serial());
        const bool ui_resource = InterlockedCompareExchange(
            &destination_state->ui_relevant, 0, 0) != 0;
        const bool mirror_full_resource = mgs4_hud::use_full_resource_mirror(
            ui_resource, destination_state->size,
            max_full_shadow_resource_size);
        const UINT64 required = mirror_full_resource
            ? destination_state->size : byte_count;
        if (ensure_shadow_capacity_locked(destination_state, required,
                                          mirror_full_resource)) {
            unsigned char* destination_bytes = destination_state->shadow +
                mgs4_hud::shadow_write_offset(
                    destination_state->shadow_full_resource,
                    destination_offset);
            std::memcpy(destination_bytes,
                        source_state->mapped + source_offset,
                        static_cast<std::size_t>(byte_count));
            if (!destination_state->shadow_full_resource) {
                destination_state->shadow_offset = destination_offset;
                destination_state->shadow_size = byte_count;
            }
        }
    }
    ReleaseSRWLockExclusive(&g_resource_lock);
    if (temporary_mapping)
        source->Unmap(0, nullptr);
}

static void STDMETHODCALLTYPE hooked_copy_buffer_region(
    ID3D12GraphicsCommandList* command_list, ID3D12Resource* destination,
    UINT64 destination_offset, ID3D12Resource* source, UINT64 source_offset,
    UINT64 byte_count) {
    shadow_buffer_copy(destination, destination_offset, source, source_offset,
                       byte_count);
    g_original_copy_buffer_region(command_list, destination, destination_offset,
                                  source, source_offset, byte_count);
}

static void STDMETHODCALLTYPE hooked_draw_instanced(
    ID3D12GraphicsCommandList* command_list, UINT vertex_count, UINT instance_count,
    UINT first_vertex, UINT first_instance) {
    CommandListState* state = command_state(command_list, false);
    auto* pipeline = state
        ? reinterpret_cast<ID3D12PipelineState*>(
            InterlockedCompareExchangePointer(
                &state->pipeline_state, nullptr, nullptr))
        : nullptr;
    const bool ui_pipeline = is_ui_pipeline(pipeline);
    const bool anchor_ui_pipeline = is_anchor_ui_pipeline(pipeline);
    const CommonUISnapshot common = ui_pipeline
        ? take_common_ui_snapshot()
        : CommonUISnapshot{};
    if (ui_pipeline)
        log_ui_draw_stack(vertex_count, first_vertex, instance_count);

    float bounds[4] = {};
    const UIAnchor anchor = (((g_anchor_ui || g_center_hud_16x9) &&
                              anchor_ui_pipeline) ||
                             ((g_ui_diagnostics || g_crosshair_diagnostics) &&
                              ui_pipeline))
        ? classify_ui_draw(state, vertex_count, first_vertex, bounds)
        : UIAnchor::Unknown;
    if (ui_pipeline) {
        log_ui_bounds(pipeline, vertex_count, first_vertex, anchor, bounds);
        log_ui_correlation(pipeline, "draw", vertex_count, common);
        log_crosshair_ui_candidate(pipeline, "draw", vertex_count, anchor,
                                   bounds, common);
    }

    D3D12_VIEWPORT safe = {};
    if (make_ui_viewport(state, &safe, anchor, vertex_count, bounds)) {
        const D3D12_VIEWPORT original = state->viewport;
        const D3D12_RECT original_scissor = state->scissor;
        D3D12_RECT safe_scissor = {};
        const bool adjust_scissor = make_ui_scissor(
            state, safe, anchor, &safe_scissor);
        g_original_rs_set_viewports(command_list, 1, &safe);
        if (adjust_scissor)
            g_original_rs_set_scissor_rects(command_list, 1, &safe_scissor);
        g_original_draw_instanced(command_list, vertex_count, instance_count,
                                  first_vertex, first_instance);
        if (adjust_scissor)
            g_original_rs_set_scissor_rects(command_list, 1, &original_scissor);
        g_original_rs_set_viewports(command_list, 1, &original);
        return;
    }
    g_original_draw_instanced(command_list, vertex_count, instance_count,
                              first_vertex, first_instance);
}

static void STDMETHODCALLTYPE hooked_draw_indexed_instanced(
    ID3D12GraphicsCommandList* command_list, UINT index_count, UINT instance_count,
    UINT first_index, INT base_vertex, UINT first_instance) {
    CommandListState* state = command_state(command_list, false);
    auto* pipeline = state
        ? reinterpret_cast<ID3D12PipelineState*>(
            InterlockedCompareExchangePointer(
                &state->pipeline_state, nullptr, nullptr))
        : nullptr;
    const bool ui_pipeline = is_ui_pipeline(pipeline);
    const CommonUISnapshot common = ui_pipeline
        ? take_common_ui_snapshot()
        : CommonUISnapshot{};
    if (ui_pipeline) {
        log_ui_draw_stack(index_count, first_index, instance_count);
        log_ui_correlation(pipeline, "indexed", index_count, common);
        log_crosshair_ui_candidate(pipeline, "indexed", index_count,
                                   UIAnchor::Unknown, nullptr, common);
    }
    D3D12_VIEWPORT safe = {};
    const float legacy_bounds[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    if (g_constrain_ui && !g_anchor_ui && !g_center_hud_16x9 &&
        make_ui_viewport(state, &safe, UIAnchor::Center, 6,
                         legacy_bounds)) {
        const D3D12_VIEWPORT original = state->viewport;
        g_original_rs_set_viewports(command_list, 1, &safe);
        g_original_draw_indexed_instanced(command_list, index_count, instance_count,
                                          first_index, base_vertex, first_instance);
        g_original_rs_set_viewports(command_list, 1, &original);
        return;
    }
    g_original_draw_indexed_instanced(command_list, index_count, instance_count,
                                      first_index, base_vertex, first_instance);
}

static bool create_and_enable_hook(void* target, void* detour, void** original,
                                   const char* name) {
    const MH_STATUS create = MH_CreateHook(target, detour, original);
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    if (create == MH_OK && enable == MH_OK) return true;
    char message[256] = {};
    std::snprintf(message, sizeof(message), "ERROR hook %s: create=%s enable=%s",
                  name, MH_StatusToString(create), MH_StatusToString(enable));
    log_line(message);
    return false;
}

static void install_command_list_hooks(ID3D12GraphicsCommandList* command_list) {
    if (!command_list || InterlockedCompareExchange(&g_d3d12_command_hooks_installed,
                                                     1, 0) != 0)
        return;
    void** vtable = *reinterpret_cast<void***>(command_list);
    // Install the two dependencies first. Never install draw detours if either
    // viewport restoration or pipeline tracking is unavailable.
    const bool viewport_ok = create_and_enable_hook(
        vtable[21], reinterpret_cast<void*>(&hooked_rs_set_viewports),
        reinterpret_cast<void**>(&g_original_rs_set_viewports), "D3D12 RSSetViewports");
    const bool scissor_ok = create_and_enable_hook(
        vtable[22], reinterpret_cast<void*>(&hooked_rs_set_scissor_rects),
        reinterpret_cast<void**>(&g_original_rs_set_scissor_rects),
        "D3D12 RSSetScissorRects");
    const bool pipeline_ok = create_and_enable_hook(
        vtable[25], reinterpret_cast<void*>(&hooked_set_pipeline_state),
        reinterpret_cast<void**>(&g_original_set_pipeline_state), "D3D12 SetPipelineState");
    bool state_ok = create_and_enable_hook(
        vtable[10], reinterpret_cast<void*>(&hooked_command_list_reset),
        reinterpret_cast<void**>(&g_original_command_list_reset), "D3D12 Reset");
    state_ok &= create_and_enable_hook(
        vtable[11], reinterpret_cast<void*>(&hooked_command_list_clear_state),
        reinterpret_cast<void**>(&g_original_command_list_clear_state), "D3D12 ClearState");
    state_ok &= create_and_enable_hook(
        vtable[15], reinterpret_cast<void*>(&hooked_copy_buffer_region),
        reinterpret_cast<void**>(&g_original_copy_buffer_region),
        "D3D12 CopyBufferRegion");
    state_ok &= create_and_enable_hook(
        vtable[44], reinterpret_cast<void*>(&hooked_ia_set_vertex_buffers),
        reinterpret_cast<void**>(&g_original_ia_set_vertex_buffers),
        "D3D12 IASetVertexBuffers");
    bool draws_ok = false;
    if (viewport_ok && scissor_ok && pipeline_ok) {
        draws_ok = create_and_enable_hook(
            vtable[12], reinterpret_cast<void*>(&hooked_draw_instanced),
            reinterpret_cast<void**>(&g_original_draw_instanced), "D3D12 DrawInstanced");
        draws_ok &= create_and_enable_hook(
            vtable[13], reinterpret_cast<void*>(&hooked_draw_indexed_instanced),
            reinterpret_cast<void**>(&g_original_draw_indexed_instanced),
            "D3D12 DrawIndexedInstanced");
    }
    const bool okay = viewport_ok && scissor_ok && pipeline_ok && state_ok && draws_ok;
    if (okay)
        log_line("D3D12: command-list hooks installed.");
    else
        InterlockedExchange(&g_d3d12_command_hooks_installed, -1);
}

static HRESULT STDMETHODCALLTYPE hooked_create_graphics_pso(
    ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
    REFIID riid, void** pipeline) {
    const HRESULT result = g_original_create_graphics_pso(device, desc, riid, pipeline);
    if (SUCCEEDED(result) && pipeline && *pipeline) {
        auto* created = reinterpret_cast<ID3D12PipelineState*>(*pipeline);
        // A newly created PSO may reuse a released COM wrapper address. Clear
        // an older UI identity even when the replacement is not a UI PSO.
        invalidate_reused_ui_pipeline(created);
        if (desc &&
            (is_ui_shader(desc->VS) || is_ui_input_layout(desc->InputLayout)))
            remember_ui_pipeline(created, desc);
    }
    return result;
}

static HRESULT STDMETHODCALLTYPE hooked_create_command_list(
    ID3D12Device* device, UINT node_mask, D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* allocator, ID3D12PipelineState* initial_state,
    REFIID riid, void** command_list) {
    const HRESULT result = g_original_create_command_list(
        device, node_mask, type, allocator, initial_state, riid, command_list);
    if (SUCCEEDED(result) && command_list && *command_list &&
        type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        auto* graphics = reinterpret_cast<ID3D12GraphicsCommandList*>(*command_list);
        if (CommandListState* state = command_state(graphics, true)) {
            InterlockedExchangePointer(&state->pipeline_state, initial_state);
            InterlockedExchange(&state->has_viewport, 0);
            InterlockedExchange(&state->has_scissor, 0);
            InterlockedExchange(&state->vertex_buffer_mask, 0);
        }
        install_command_list_hooks(graphics);
    }
    return result;
}

static void log_crosshair_resource(const char* allocation,
                                   const D3D12_RESOURCE_DESC* desc,
                                   D3D12_RESOURCE_STATES initial_state,
                                   const D3D12_CLEAR_VALUE* clear_value,
                                   D3D12_HEAP_TYPE heap_type) {
    if (!g_crosshair_diagnostics || !desc ||
        desc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        return;

    const UINT64 width = desc->Width;
    const UINT height = desc->Height;
    const bool resolution_related =
        width == g_target_width || height == g_target_height ||
        width == 4096 || height == 4096 || width == 2048 || height == 2048 ||
        width >= 4000 || height >= 2000;
    if (!resolution_related) return;

    std::uint64_t key = 1469598103934665603ull;
    const std::uint64_t values[] = {
        width, height, desc->DepthOrArraySize, desc->MipLevels,
        static_cast<std::uint64_t>(desc->Format), desc->SampleDesc.Count,
        static_cast<std::uint64_t>(desc->Flags),
        static_cast<std::uint64_t>(initial_state),
        static_cast<std::uint64_t>(heap_type),
    };
    for (std::uint64_t value : values) {
        key ^= value;
        key *= 1099511628211ull;
    }
    if (!key) key = 1;
    static volatile LONG64 keys[512];
    const std::size_t start = static_cast<std::size_t>(key % 512);
    for (std::size_t probe = 0; probe < 512; ++probe) {
        volatile LONG64* slot = &keys[(start + probe) % 512];
        const LONG64 old = InterlockedCompareExchange64(
            slot, static_cast<LONG64>(key), 0);
        if (old == static_cast<LONG64>(key)) return;
        if (old == 0) break;
    }

    char message[384] = {};
    std::snprintf(message, sizeof(message),
        "CROSSHAIR-RESOURCE allocation=%s size=%llux%u array=%u mips=%u "
        "format=%u samples=%u flags=0x%x initial=0x%x heap=%u clear_format=%u",
        allocation, static_cast<unsigned long long>(width), height,
        desc->DepthOrArraySize, desc->MipLevels,
        static_cast<unsigned>(desc->Format), desc->SampleDesc.Count,
        static_cast<unsigned>(desc->Flags),
        static_cast<unsigned>(initial_state), static_cast<unsigned>(heap_type),
        clear_value ? static_cast<unsigned>(clear_value->Format) : 0u);
    log_line(message);
}

static HRESULT STDMETHODCALLTYPE hooked_create_committed_resource(
    ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap_properties,
    D3D12_HEAP_FLAGS heap_flags, const D3D12_RESOURCE_DESC* desc,
    D3D12_RESOURCE_STATES initial_state, const D3D12_CLEAR_VALUE* clear_value,
    REFIID riid, void** resource) {
    log_crosshair_resource("committed", desc, initial_state, clear_value,
                           heap_properties ? heap_properties->Type
                                           : D3D12_HEAP_TYPE_CUSTOM);
    const HRESULT result = g_original_create_committed_resource(
        device, heap_properties, heap_flags, desc, initial_state, clear_value,
        riid, resource);
    if (SUCCEEDED(result) && resource && *resource)
        install_resource_hooks(reinterpret_cast<ID3D12Resource*>(*resource));
    return result;
}

static HRESULT STDMETHODCALLTYPE hooked_create_placed_resource(
    ID3D12Device* device, ID3D12Heap* heap, UINT64 heap_offset,
    const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES initial_state,
    const D3D12_CLEAR_VALUE* clear_value, REFIID riid, void** resource) {
    log_crosshair_resource("placed", desc, initial_state, clear_value,
                           D3D12_HEAP_TYPE_CUSTOM);
    const HRESULT result = g_original_create_placed_resource(
        device, heap, heap_offset, desc, initial_state, clear_value, riid,
        resource);
    if (SUCCEEDED(result) && resource && *resource)
        install_resource_hooks(reinterpret_cast<ID3D12Resource*>(*resource));
    return result;
}

static void install_d3d12_device_hooks(ID3D12Device* device) {
    if (!device || InterlockedCompareExchange(&g_d3d12_device_hooks_installed, 1, 0) != 0)
        return;
    void** vtable = *reinterpret_cast<void***>(device);
    const bool pso_ok = create_and_enable_hook(
        vtable[10], reinterpret_cast<void*>(&hooked_create_graphics_pso),
        reinterpret_cast<void**>(&g_original_create_graphics_pso),
        "D3D12 CreateGraphicsPipelineState");
    const bool lists_ok = create_and_enable_hook(
        vtable[12], reinterpret_cast<void*>(&hooked_create_command_list),
        reinterpret_cast<void**>(&g_original_create_command_list),
        "D3D12 CreateCommandList");
    const bool placed_ok = create_and_enable_hook(
        vtable[29], reinterpret_cast<void*>(&hooked_create_placed_resource),
        reinterpret_cast<void**>(&g_original_create_placed_resource),
        "D3D12 CreatePlacedResource");
    const bool committed_ok = create_and_enable_hook(
        vtable[27], reinterpret_cast<void*>(&hooked_create_committed_resource),
        reinterpret_cast<void**>(&g_original_create_committed_resource),
        "D3D12 CreateCommittedResource");
    const bool okay = pso_ok && lists_ok && placed_ok && committed_ok;
    if (okay)
        log_line("D3D12: device hooks installed.");
    else
        InterlockedExchange(&g_d3d12_device_hooks_installed, -1);
}

static HRESULT WINAPI hooked_d3d12_create_device(IUnknown* adapter,
                                                  D3D_FEATURE_LEVEL minimum_level,
                                                  REFIID riid, void** device) {
    const HRESULT result = g_original_d3d12_create_device(adapter, minimum_level,
                                                           riid, device);
    if (SUCCEEDED(result) && device && *device)
        install_d3d12_device_hooks(reinterpret_cast<ID3D12Device*>(*device));
    return result;
}

static bool ensure_d3d12_export_hook() {
    const LONG state = InterlockedCompareExchange(&g_d3d12_export_hook_state, 1, 0);
    if (state == 2) return true;
    if (state == -1) return false;
    if (state == 1) {
        while (InterlockedCompareExchange(&g_d3d12_export_hook_state, 0, 0) == 1)
            Sleep(0);
        return InterlockedCompareExchange(&g_d3d12_export_hook_state, 0, 0) == 2;
    }

    // Force the runtime to load from the first synchronous winmm call.  A
    // background polling thread can lose the race against renderer creation.
    HMODULE d3d12 = LoadLibraryW(L"d3d12.dll");
    if (!d3d12) {
        log_line("D3D12 is not loaded; the active backend may be D3D11.");
        InterlockedExchange(&g_d3d12_export_hook_state, -1);
        return false;
    }
    void* target = reinterpret_cast<void*>(GetProcAddress(d3d12, "D3D12CreateDevice"));
    if (!target || !initialize_minhook() ||
        !create_and_enable_hook(target, reinterpret_cast<void*>(&hooked_d3d12_create_device),
                                reinterpret_cast<void**>(&g_original_d3d12_create_device),
                                "D3D12CreateDevice")) {
        log_line("ERROR: could not intercept D3D12 device creation.");
        InterlockedExchange(&g_d3d12_export_hook_state, -1);
        return false;
    }
    InterlockedExchange(&g_d3d12_export_hook_state, 2);
    log_line("D3D12CreateDevice intercepted before renderer creation.");
    return true;
}

static DWORD WINAPI render_backend_hook_thread(void*) {
    // Resource creation starts before the main patch thread on some systems.
    // Read this passive flag here as well so the high-resolution inventory
    // cannot miss the first depth/auxiliary targets.
    char ini_path[MAX_PATH] = {};
    if (!game_sibling_path(ini_path, settings_filename())) return 0;
#ifdef MGS4_LAB_ONLY
    if (GetPrivateProfileIntA("Lab", "Enabled", 0, ini_path) == 0) return 0;
    g_target_width = GetPrivateProfileIntA(
        "Ultrawide", "Width", 3440, ini_path);
    g_target_height = GetPrivateProfileIntA(
        "Ultrawide", "Height", 1440, ini_path);
    g_constrain_ui = GetPrivateProfileIntA(
        "Ultrawide", "ConstrainUITo16x9", 0, ini_path) != 0;
    g_anchor_ui = GetPrivateProfileIntA(
        "Ultrawide", "AnchorUIToSafeArea", 0, ini_path) != 0;
    g_center_hud_16x9 = GetPrivateProfileIntA(
        "Ultrawide", "CenterHUDIn16x9", 0, ini_path) != 0;
    g_ui_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "UIDiagnostics", 0, ini_path) != 0;
#endif
    g_crosshair_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "CrosshairDiagnostics", 0, ini_path) != 0;
#ifdef MGS4_LAB_ONLY
    if (!g_constrain_ui && !g_anchor_ui && !g_center_hud_16x9 &&
        !g_ui_diagnostics && !g_crosshair_diagnostics)
        return 0;
#endif
    ensure_d3d12_export_hook();
    return 0;
}

static std::uint64_t __fastcall hooked_player_sight_update(void* sight) {
    const std::uint64_t result = g_original_player_sight_update(sight);
    if (!g_crosshair_diagnostics || !sight) return result;

    const LONG sample = InterlockedIncrement(&g_crosshair_sight_samples);
    // Roughly twice per second at 60 FPS. The output is passive and intended
    // to be compared while aiming near/far at one fixed camera position.
    if (sample > 4096 || ((sample - 1) % 30) != 0) return result;

    const auto* bytes = static_cast<const unsigned char*>(sight);
    const auto* target = reinterpret_cast<const float*>(bytes + 0x60);
    const auto* origin = reinterpret_cast<const float*>(bytes + 0x70);
    const auto* direction = reinterpret_cast<const float*>(bytes + 0x80);
    const unsigned flags = bytes[0xb0];
    char message[448] = {};
    std::snprintf(message, sizeof(message),
        "CROSSHAIR-SIGHT sample=%ld tick=%llu result=0x%llx flags=0x%02x "
        "target=(%.6f,%.6f,%.6f,%.6f) origin=(%.6f,%.6f,%.6f,%.6f) "
        "direction=(%.6f,%.6f,%.6f,%.6f)",
        sample, static_cast<unsigned long long>(GetTickCount64()),
        static_cast<unsigned long long>(result), flags,
        target[0], target[1], target[2], target[3],
        origin[0], origin[1], origin[2], origin[3],
        direction[0], direction[1], direction[2], direction[3]);
    log_line(message);
    return result;
}

static bool install_player_sight_diagnostics(std::uintptr_t base) {
    if (!g_crosshair_diagnostics) return true;
    constexpr std::uintptr_t player_sight_update_rva = 0x97bc60;
    void* sight_target = reinterpret_cast<void*>(base + player_sight_update_rva);
    const bool okay = create_and_enable_hook(
        sight_target, reinterpret_cast<void*>(&hooked_player_sight_update),
        reinterpret_cast<void**>(&g_original_player_sight_update),
        "PL_PLG_SIGHT update diagnostics");
    if (okay)
        log_line("Crosshair diagnostics: sight endpoint and common-UI correlation enabled; no game state is modified.");
    return okay;
}

extern "C" __declspec(dllexport) MMRESULT WINAPI timeBeginPeriod(UINT period) {
    ensure_d3d12_export_hook();
    if (!g_real_winmm) g_real_winmm = LoadLibraryW(L"C:\\windows\\system32\\winmm.dll");
    auto fn = reinterpret_cast<TimeBeginPeriodFn>(GetProcAddress(g_real_winmm, "timeBeginPeriod"));
    return fn ? fn(period) : static_cast<MMRESULT>(0); // TIMERR_NOERROR
}

extern "C" __declspec(dllexport) DWORD WINAPI timeGetTime() {
    ensure_d3d12_export_hook();
    if (!g_real_winmm) g_real_winmm = LoadLibraryW(L"C:\\windows\\system32\\winmm.dll");
    auto fn = reinterpret_cast<TimeGetTimeFn>(GetProcAddress(g_real_winmm, "timeGetTime"));
    return fn ? fn() : GetTickCount();
}

static bool near_zero(float x) {
    return std::isfinite(x) && std::fabs(x) < 0.00001f;
}

static void log_projection_diagnostic(float x, float y, float source_aspect,
                                      bool finite_scale,
                                      bool perspective_shape,
                                      bool known_aspect,
                                      bool patched) {
    if (!g_projection_diagnostics) return;

    void* frames[16] = {};
    const USHORT frame_count = RtlCaptureStackBackTrace(
        0, static_cast<ULONG>(sizeof(frames) / sizeof(frames[0])), frames, nullptr);
    std::uintptr_t caller_rva = 0;
    if (g_executable_base) {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_executable_base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
            g_executable_base + static_cast<std::uintptr_t>(dos->e_lfanew));
        const std::uintptr_t executable_end =
            g_executable_base + nt->OptionalHeader.SizeOfImage;
        for (USHORT index = 0; index < frame_count; ++index) {
            const auto address = reinterpret_cast<std::uintptr_t>(frames[index]);
            if (address >= g_executable_base && address < executable_end) {
                caller_rva = address - g_executable_base;
                break;
            }
        }
    }

    std::uint64_t key = 1469598103934665603ull;
    const std::int64_t values[] = {
        static_cast<std::int64_t>(caller_rva),
        static_cast<std::int64_t>(x * 10000.0f),
        static_cast<std::int64_t>(y * 10000.0f),
        static_cast<std::int64_t>(source_aspect * 10000.0f),
        finite_scale ? 1 : 0, perspective_shape ? 1 : 0,
        known_aspect ? 1 : 0, patched ? 1 : 0,
    };
    for (const std::int64_t value : values) {
        key ^= static_cast<std::uint64_t>(value);
        key *= 1099511628211ull;
    }
    if (!key) key = 1;
    static volatile LONG64 keys[512];
    const std::size_t start = static_cast<std::size_t>(key % 512);
    for (std::size_t probe = 0; probe < 512; ++probe) {
        volatile LONG64* slot = &keys[(start + probe) % 512];
        const LONG64 old = InterlockedCompareExchange64(
            slot, static_cast<LONG64>(key), 0);
        if (old == static_cast<LONG64>(key)) return;
        if (old == 0) break;
    }

    static volatile LONG sequence;
    const LONG number = InterlockedIncrement(&sequence);
    if (number > 512) return;
    char message[320] = {};
    std::snprintf(message, sizeof(message),
        "PROJECTION-DIAG #%ld caller=0x%llx x=%.7f y=%.7f source_aspect=%.7f "
        "target_aspect=%.7f finite=%s perspective=%s known=%s patched=%s",
        number, static_cast<unsigned long long>(caller_rva), x, y,
        source_aspect, g_target_aspect,
        finite_scale ? "yes" : "no",
        perspective_shape ? "yes" : "no",
        known_aspect ? "yes" : "no", patched ? "yes" : "no");
    log_line(message);
}

// Engine-level projection setter, used before rendering branches into D3D11
// or D3D12. FOVMultiplier scales the tangent of the vertical FOV; deriving X
// from that adjusted Y and the target aspect preserves geometry proportions.
static void __fastcall hooked_set_projection(const float* matrix) {
    if (!matrix) {
        g_original_set_projection(matrix);
        return;
    }

    float copy[16];
    std::memcpy(copy, matrix, sizeof(copy));
    const float x = copy[0];
    const float y = copy[5];
    // The handoff candidate 341084b captured and visually validated a real
    // close-up at m00=5.6216335/m11=-13.4294577. Upper ceilings are therefore
    // not valid projection tests. The complete structure and known aspect are
    // the safety boundary; retain only the established non-zero lower bound.
    const bool finite_scale = std::isfinite(x) && std::isfinite(y) &&
                              std::fabs(x) >= 0.25f &&
                              std::fabs(y) >= 0.25f;
    const bool perspective_shape = near_zero(copy[1]) && near_zero(copy[2]) &&
        near_zero(copy[3]) && near_zero(copy[4]) && near_zero(copy[6]) &&
        near_zero(copy[7]) && near_zero(copy[8]) && near_zero(copy[9]) &&
        near_zero(copy[12]) && near_zero(copy[13]) && near_zero(copy[15]) &&
        std::isfinite(copy[10]) && std::isfinite(copy[14]) &&
        std::fabs(std::fabs(copy[11]) - 1.0f) < 0.0002f;
    const float source_aspect = finite_scale ? std::fabs(y / x) : 0.0f;
    const bool known_aspect =
        std::fabs(source_aspect - (16.0f / 9.0f)) < 0.0003f ||
        std::fabs(source_aspect - g_target_aspect) < 0.0003f;
    const bool patch = finite_scale && perspective_shape && known_aspect;
    log_projection_diagnostic(x, y, source_aspect, finite_scale,
                              perspective_shape, known_aspect, patch);
    if (patch) {
        const auto route_mode = current_camera_route_test_mode();
        if (mgs4_fov::renderer_owns_fov(
                g_native_camera_fov_active, g_camera_route_test_enabled,
                route_mode)) {
            copy[5] = std::copysign(std::fabs(y) / g_fov_multiplier, y);
            copy[0] = std::copysign(std::fabs(copy[5]) / g_target_aspect, x);
        } else {
            mgs4_fov::correct_aspect_only(copy, g_target_aspect);
        }
        InterlockedIncrement(&g_projection_patches);
    }
    g_original_set_projection(copy);
}

// Native camera-FOV path. Adjust the scale before FUN_1400b9bb0 constructs its
// three projections, two combined matrices and six normalized visibility
// planes. Nothing in the camera object is rewritten after the original returns.
static void __fastcall hooked_build_camera(void* camera, const void* source,
                                            float projection_scale,
                                            float parameter4, float parameter5,
                                            float aspect_scale) {
    const auto return_address = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::uintptr_t caller_rva =
        return_address >= g_executable_base
            ? return_address - g_executable_base
            : return_address;
    std::size_t owner_index = std::size(kCameraBuilderCallerRvas);
    for (std::size_t i = 0; i < std::size(kCameraBuilderCallerRvas); ++i) {
        if (caller_rva == kCameraBuilderCallerRvas[i]) {
            owner_index = i;
            break;
        }
    }
    const auto route_mode = current_camera_route_test_mode();
    const auto exclusion_mask = static_cast<std::uint64_t>(
        InterlockedCompareExchange64(&g_camera_route_exclusion_mask, 0, 0));
    const bool direct_cinematic_owner =
        g_cinematic_camera_owner_depth != 0 &&
        owner_index == mgs4_fov::kNativeFovCinematicWrapperRouteIndex;
    const ULONGLONG now_tick = GetTickCount64();
    const bool recent_cinematic_camera =
        !g_camera_route_test_enabled && g_native_camera_fov_requested &&
        camera == g_cinematic_camera_object &&
        now_tick - g_cinematic_camera_tick <= 250 &&
        mgs4_fov::is_cinematic_rebuild_route(owner_index);
    const float propagation_tolerance =
        std::fmax(0.001f, std::fabs(g_cinematic_camera_input_scale) * 0.001f);
    const bool propagate_cinematic_fov =
        recent_cinematic_camera &&
        std::fabs(projection_scale - g_cinematic_camera_input_scale) <=
            propagation_tolerance;
    const bool apply_native_fov = propagate_cinematic_fov ||
        mgs4_fov::should_apply_native_camera_fov(
            g_native_camera_fov_requested, g_camera_route_test_enabled,
            route_mode, owner_index, std::size(kCameraBuilderCallerRvas),
            exclusion_mask, direct_cinematic_owner);
    const float adjusted = mgs4_fov::camera_scale_for_mode(
        projection_scale, g_fov_multiplier, apply_native_fov);
    if (direct_cinematic_owner) {
        g_cinematic_camera_object = camera;
        g_cinematic_camera_tick = now_tick;
        g_cinematic_camera_input_scale = projection_scale;
    }
    if (adjusted != projection_scale)
        InterlockedIncrement(&g_native_camera_fov_patches);
    g_original_build_camera(camera, source, adjusted, parameter4, parameter5,
                            aspect_scale);

    if (!g_camera_ownership_diagnostics) return;

    CameraOwnershipSlot& slot = g_camera_ownership_slots[owner_index];
    const LONG64 calls = InterlockedIncrement64(&slot.calls);
    const LONG64 now = static_cast<LONG64>(GetTickCount64());
    const LONG64 last = InterlockedCompareExchange64(&slot.last_log_ms, 0, 0);
    if (calls != 1 && now - last < 500) return;
    if (InterlockedCompareExchange64(&slot.last_log_ms, now, last) != last)
        return;

    std::uint32_t viewport_width = 0;
    std::uint32_t viewport_height = 0;
    std::uint32_t flags = 0;
    if (camera) {
        const auto* bytes = static_cast<const unsigned char*>(camera);
        std::memcpy(&viewport_width, bytes + 0x360, sizeof(viewport_width));
        std::memcpy(&viewport_height, bytes + 0x364, sizeof(viewport_height));
        std::memcpy(&flags, bytes + 0x394, sizeof(flags));
    }
    const LONG sample = InterlockedIncrement(&g_camera_ownership_samples);
    void* stack[16] = {};
    const USHORT stack_count = RtlCaptureStackBackTrace(
        0, static_cast<ULONG>(std::size(stack)), stack, nullptr);
    char stack_rvas[320] = {};
    std::size_t stack_offset = 0;
    for (USHORT i = 0; i < stack_count; ++i) {
        const auto address = reinterpret_cast<std::uintptr_t>(stack[i]);
        if (address < g_executable_base ||
            address >= g_executable_base + kSupportedExecutableSize) {
            continue;
        }
        const int written = std::snprintf(
            stack_rvas + stack_offset, sizeof(stack_rvas) - stack_offset,
            "%s0x%llx", stack_offset ? "," : "",
            static_cast<unsigned long long>(address - g_executable_base));
        if (written <= 0 ||
            static_cast<std::size_t>(written) >=
                sizeof(stack_rvas) - stack_offset) {
            break;
        }
        stack_offset += static_cast<std::size_t>(written);
    }

    char message[800] = {};
    std::snprintf(message, sizeof(message),
        "CAMERA-ACTIVE #%ld time=%llu caller_return_rva=0x%llx calls=%llu "
        "camera=%p source=%p viewport=%ux%u flags=0x%08x "
        "input_scale=%.7f effective_scale=%.7f p4=%.7f p5=%.7f "
        "aspect_scale=%.7f route=%u mode=%s excluded=%s "
        "native_fov_applied=%s stack_rvas=[%s]",
        sample, static_cast<unsigned long long>(now),
        static_cast<unsigned long long>(caller_rva),
        static_cast<unsigned long long>(calls), camera, source,
        viewport_width, viewport_height, flags, projection_scale, adjusted,
        parameter4, parameter5, aspect_scale,
        owner_index < std::size(kCameraBuilderCallerRvas)
            ? static_cast<unsigned>(owner_index + 1) : 0,
        camera_route_mode_name(route_mode),
        owner_index < std::size(kCameraBuilderCallerRvas) &&
                (exclusion_mask & (std::uint64_t{1} << owner_index))
            ? "yes" : "no",
        apply_native_fov ? "yes" : "no", stack_rvas);
    log_line(message);
}

// FUN_140652e00 owns the camera path used by the in-engine cinematic captured
// at return RVA 0x653b99. It reaches the common builder through wrapper route
// 02, which also has many unrelated owners, so a TLS scope identifies only
// this high-level call chain. This avoids stack walking in production and does
// not expose WeaponWindow or auxiliary render-target cameras to native FOV.
static void __fastcall hooked_update_cinematic_camera(void* context) {
    ++g_cinematic_camera_owner_depth;
    g_original_update_cinematic_camera(context);
    --g_cinematic_camera_owner_depth;
}

// Central display-mode setter.  The original routine is still allowed to do
// all backend and window management, but it always receives the configured
// native size.  Once it has finished its built-in 16:9 safe-area calculation,
// replace that final viewport with the native ultrawide canvas exactly once
// per real mode change.
static void __fastcall hooked_set_resolution(std::uint16_t mode,
                                             std::int32_t index,
                                             std::uint8_t use_safe_area,
                                             std::uint32_t,
                                             std::uint32_t) {
    g_original_set_resolution(mode, index, use_safe_area,
                              g_target_width, g_target_height);
    apply_resolution_state();
}

static bool supported_executable(std::uintptr_t base) {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    return nt->Signature == IMAGE_NT_SIGNATURE && nt->FileHeader.TimeDateStamp == 0x6a8cfc47 &&
           nt->OptionalHeader.SizeOfImage == kSupportedExecutableSize;
}

// The port maps its current input-device kind to a complete prompt profile.
// Under Proton it can keep returning profile 0 (keyboard) even with a live
// XInput slot. Return profile 1 (Xbox) so atlas and symbol map change together;
// input handling itself is left untouched.
static bool force_xbox_prompts(std::uintptr_t base) {
    constexpr std::uintptr_t prompt_profile_selector_rva = 0x42da20;
    constexpr unsigned char expected[] = {0x48, 0x83, 0xec, 0x28, 0xe8};
    constexpr unsigned char replacement[] = {0xb8, 0x01, 0x00, 0x00, 0x00, 0xc3};
    auto* target = reinterpret_cast<unsigned char*>(base + prompt_profile_selector_rva);
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        if (std::memcmp(target, expected, sizeof(expected)) == 0) break;
        if (attempt == 199) {
            log_line("ERROR: prompt-profile selector did not decrypt in time.");
            return false;
        }
        Sleep(25);
    }
    if (std::memcmp(target, expected, sizeof(expected)) != 0) {
        log_line("ERROR: prompt-profile selector not recognized; Xbox icons were not forced.");
        return false;
    }

    DWORD old_protection = 0;
    if (!VirtualProtect(target, sizeof(replacement), PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not make the prompt-profile selector writable.");
        return false;
    }
    std::memcpy(target, replacement, sizeof(replacement));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(replacement));
    DWORD ignored = 0;
    VirtualProtect(target, sizeof(replacement), old_protection, &ignored);
    log_line("Iconos Xbox forzados: selector de perfil fijado a XBox_Pad.");
    return true;
}

// The game copies these two compositor getters into its global render size
// during startup.  The proxy thread used to race that initialization: the
// compositor was corrected afterwards, but the global could remain 2560x1440
// for the whole session.  Cinematic/post-processing passes then sampled a
// 2560-wide surface into a 3440-wide output, corrupting the extra right-hand
// region.  Replace the trivial getters before initialization can copy them.
static bool force_resolution_getters(std::uintptr_t base,
                                     std::uint32_t width,
                                     std::uint32_t height) {
    struct GetterPatch {
        std::uintptr_t rva;
        const unsigned char* expected;
        std::uint32_t value;
        const char* name;
    };
    constexpr unsigned char width_expected[] =
        {0x8b, 0x05, 0x32, 0x51, 0x57, 0x03, 0xc3};
    constexpr unsigned char height_expected[] =
        {0x8b, 0x05, 0x46, 0x51, 0x57, 0x03, 0xc3};
    const GetterPatch patches[] = {
        {0x65c040, width_expected, width, "width"},
        {0x65c030, height_expected, height, "height"},
    };

    for (const GetterPatch& patch : patches) {
        auto* target = reinterpret_cast<unsigned char*>(base + patch.rva);
        for (unsigned attempt = 0; attempt < 200; ++attempt) {
            if (std::memcmp(target, patch.expected, 7) == 0) break;
            if (attempt == 199) {
                log_line("ERROR: a resolution getter did not decrypt in time.");
                return false;
            }
            Sleep(25);
        }
        unsigned char replacement[] = {0xb8, 0, 0, 0, 0, 0xc3};
        std::memcpy(replacement + 1, &patch.value, sizeof(patch.value));
        DWORD old_protection = 0;
        if (!VirtualProtect(target, sizeof(replacement), PAGE_EXECUTE_READWRITE,
                            &old_protection)) {
            log_line("ERROR: could not write a resolution getter.");
            return false;
        }
        std::memcpy(target, replacement, sizeof(replacement));
        FlushInstructionCache(GetCurrentProcess(), target, sizeof(replacement));
        DWORD ignored = 0;
        VirtualProtect(target, sizeof(replacement), old_protection, &ignored);
    }
    log_line("Resolution getters fixed before surface initialization.");
    return true;
}

static bool initialize_minhook() {
    const LONG state = InterlockedCompareExchange(&g_minhook_state, 1, 0);
    if (state == 0) {
        const MH_STATUS status = MH_Initialize();
        const bool okay = status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED;
        InterlockedExchange(&g_minhook_state, okay ? 2 : -1);
        return okay;
    }
    while (InterlockedCompareExchange(&g_minhook_state, 0, 0) == 1) Sleep(0);
    return InterlockedCompareExchange(&g_minhook_state, 0, 0) == 2;
}

static bool install_xinput_diagnostics() {
    HMODULE xinput = LoadLibraryW(L"xinput1_4.dll");
    void* target = xinput
        ? reinterpret_cast<void*>(GetProcAddress(xinput, "XInputGetState"))
        : nullptr;
    if (!target || !initialize_minhook() ||
        !create_and_enable_hook(target,
                                reinterpret_cast<void*>(&hooked_xinput_get_state),
                                reinterpret_cast<void**>(&g_original_xinput_get_state),
                                "XInputGetState diagnostic")) {
        log_line("ERROR: XInput diagnostics hook could not be installed.");
        return false;
    }
    log_line("XInput diagnostics hook installed.");
    return true;
}

static bool wait_for_runtime_code(std::uintptr_t base, std::uintptr_t rva,
                                  const unsigned char* expected,
                                  std::size_t expected_size) {
    const auto* target = reinterpret_cast<const unsigned char*>(base + rva);
    for (unsigned attempt = 0; attempt < 400; ++attempt) {
        if (std::memcmp(target, expected, expected_size) == 0) return true;
        Sleep(25);
    }
    return false;
}

static bool install_ui_common_diagnostics(std::uintptr_t base) {
    constexpr std::uintptr_t common_ui_flush_rva = 0x79f2b0;
    constexpr unsigned char expected[] =
        {0x48, 0x89, 0x5c, 0x24, 0x20, 0x55, 0x56, 0x41};
    if (!wait_for_runtime_code(base, common_ui_flush_rva, expected,
                               sizeof(expected))) {
        log_line("ERROR: common UI emitter did not decrypt in time.");
        return false;
    }
    if (!initialize_minhook()) {
        log_line("ERROR: MinHook initialization failed for common UI diagnostics.");
        return false;
    }

    auto* target = reinterpret_cast<void*>(base + common_ui_flush_rva);
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the common UI diagnostic hook.");
        return false;
    }
    const bool okay = create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_common_ui_flush),
        reinterpret_cast<void**>(&g_original_common_ui_flush),
        "common UI emitter diagnostic");
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (okay)
        log_line("Common UI emitter diagnostics installed without data mutation.");
    return okay;
}

static bool install_cinematic_diagnostics(std::uintptr_t base) {
    constexpr std::uintptr_t get_fps_rva = 0x140a60;
    constexpr unsigned char expected[] =
        {0x40, 0x55, 0x48, 0x8b, 0xec, 0x48, 0x83, 0xec};
    if (!wait_for_runtime_code(base, get_fps_rva, expected, sizeof(expected))) {
        log_line("ERROR: FPS getter did not decrypt in time for cinematic diagnostics.");
        return false;
    }
    if (!initialize_minhook()) {
        log_line("ERROR: MinHook initialization failed for cinematic diagnostics.");
        return false;
    }

    auto* target = reinterpret_cast<void*>(base + get_fps_rva);
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the passive FPS diagnostic hook.");
        return false;
    }
    const bool okay = create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_get_fps),
        reinterpret_cast<void**>(&g_original_get_fps),
        "FPS/Bink passive diagnostic");
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (okay)
        log_line("FPS/Bink diagnostics installed; getter return value is unchanged.");
    return okay;
}

static bool dump_runtime_region(std::uintptr_t base, std::uintptr_t rva,
                                std::size_t size, const char* filename) {
    char path[MAX_PATH] = {};
    if (!game_sibling_path(path, filename)) return false;
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool okay = WriteFile(file, reinterpret_cast<const void*>(base + rva),
                                static_cast<DWORD>(size), &written, nullptr) != FALSE &&
                      written == size;
    CloseHandle(file);
    return okay;
}

// Diagnostic-only hooks around the game's own GV_PAD worker. XInput activity
// alone cannot tell whether the port converts and publishes that state. These
// counters reveal the exact stage that stalls without altering controller data.
static bool install_gamepad_pipeline_diagnostics(std::uintptr_t base) {
    constexpr std::uintptr_t convert_rva = 0x743370;
    constexpr std::uintptr_t update_rva = 0x743ed0;
    constexpr std::uintptr_t publish_rva = 0x746000;
    constexpr std::uintptr_t keyboard_merge_rva = 0x74fc10;
    constexpr unsigned char convert_expected[] =
        {0x40, 0x57, 0x48, 0x83, 0xec, 0x50};
    constexpr unsigned char update_expected[] =
        {0x44, 0x89, 0x4c, 0x24, 0x20, 0x4c, 0x89, 0x44};
    constexpr unsigned char publish_expected[] =
        {0x48, 0x89, 0x5c, 0x24, 0x08, 0x55, 0x56, 0x57};
    constexpr unsigned char keyboard_merge_expected[] =
        {0x48, 0x8b, 0xc4, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55};
    if (!wait_for_runtime_code(base, convert_rva, convert_expected,
                               sizeof(convert_expected)) ||
        !wait_for_runtime_code(base, update_rva, update_expected,
                               sizeof(update_expected)) ||
        !wait_for_runtime_code(base, publish_rva, publish_expected,
                               sizeof(publish_expected)) ||
        !wait_for_runtime_code(base, keyboard_merge_rva,
                               keyboard_merge_expected,
                               sizeof(keyboard_merge_expected))) {
        log_line("ERROR: gamepad pipeline did not decrypt in time.");
        return false;
    }
    const bool convert_dumped = dump_runtime_region(
        base, 0x743000, 0x1000, "mgs4_pad_convert_runtime.bin");
    const bool update_dumped = dump_runtime_region(
        base, 0x743e00, 0x1800, "mgs4_pad_update_runtime.bin");
    const bool selector_dumped = dump_runtime_region(
        base, 0x750000, 0x2000, "mgs4_input_selector_runtime.bin");
    if (convert_dumped && update_dumped && selector_dumped)
        log_line("Decrypted gamepad runtime regions dumped for analysis.");
    else
        log_line("WARNING: one or more gamepad runtime dumps failed.");
    if (!initialize_minhook()) {
        log_line("ERROR: MinHook initialization failed for gamepad diagnostics.");
        return false;
    }
    auto* convert_target = reinterpret_cast<void*>(base + convert_rva);
    auto* update_target = reinterpret_cast<void*>(base + update_rva);
    auto* publish_target = reinterpret_cast<void*>(base + publish_rva);
    auto* keyboard_merge_target = reinterpret_cast<void*>(base + keyboard_merge_rva);
    DWORD convert_protection = 0;
    DWORD update_protection = 0;
    DWORD publish_protection = 0;
    DWORD keyboard_merge_protection = 0;
    const bool convert_writable = VirtualProtect(
        convert_target, 32, PAGE_EXECUTE_READWRITE, &convert_protection) != FALSE;
    const bool update_writable = VirtualProtect(
        update_target, 32, PAGE_EXECUTE_READWRITE, &update_protection) != FALSE;
    const bool publish_writable = VirtualProtect(
        publish_target, 32, PAGE_EXECUTE_READWRITE, &publish_protection) != FALSE;
    const bool keyboard_merge_writable = VirtualProtect(
        keyboard_merge_target, 32, PAGE_EXECUTE_READWRITE,
        &keyboard_merge_protection) != FALSE;
    if (!convert_writable || !update_writable || !publish_writable ||
        !keyboard_merge_writable) {
        DWORD ignored = 0;
        if (convert_writable)
            VirtualProtect(convert_target, 32, convert_protection, &ignored);
        if (update_writable)
            VirtualProtect(update_target, 32, update_protection, &ignored);
        if (publish_writable)
            VirtualProtect(publish_target, 32, publish_protection, &ignored);
        if (keyboard_merge_writable)
            VirtualProtect(keyboard_merge_target, 32, keyboard_merge_protection,
                           &ignored);
        log_line("ERROR: could not enable gamepad diagnostic hooks.");
        return false;
    }
    const bool convert_ok = create_and_enable_hook(
        convert_target,
        reinterpret_cast<void*>(&hooked_pad_convert),
        reinterpret_cast<void**>(&g_original_pad_convert),
        "gamepad state converter diagnostic");
    const bool update_ok = create_and_enable_hook(
        update_target,
        reinterpret_cast<void*>(&hooked_pad_update),
        reinterpret_cast<void**>(&g_original_pad_update),
        "gamepad worker diagnostic");
    const bool publish_ok = create_and_enable_hook(
        publish_target,
        reinterpret_cast<void*>(&hooked_pad_publish),
        reinterpret_cast<void**>(&g_original_pad_publish),
        "gamepad publisher diagnostic");
    const bool keyboard_merge_ok = create_and_enable_hook(
        keyboard_merge_target,
        reinterpret_cast<void*>(&hooked_keyboard_merge),
        reinterpret_cast<void**>(&g_original_keyboard_merge),
        "keyboard merge diagnostic");
    DWORD ignored = 0;
    VirtualProtect(convert_target, 32, convert_protection, &ignored);
    VirtualProtect(update_target, 32, update_protection, &ignored);
    VirtualProtect(publish_target, 32, publish_protection, &ignored);
    VirtualProtect(keyboard_merge_target, 32, keyboard_merge_protection, &ignored);
    if (convert_ok && update_ok && publish_ok && keyboard_merge_ok)
        log_line("Gamepad pipeline diagnostics installed without state mutation.");
    return convert_ok && update_ok && publish_ok && keyboard_merge_ok;
}

// Minimal production hook for the port's controller-selection regression. The
// diagnostic build proves GV_PAD publishes correct axes, after which spurious
// keyboard detection invokes FUN_14074fc10 and neutralizes them. Correct the
// detected profile at its sole setter: retain any native controller family
// (profiles 1..7) while its slot remains connected. No launch parameter,
// controller state synthesis or periodic rewrite is involved.
static bool install_controller_input_lock(std::uintptr_t base) {
    constexpr std::uintptr_t set_detected_profile_rva = 0x750ec0;
    constexpr unsigned char expected[] =
        {0x40, 0x53, 0x48, 0x83, 0xec, 0x20, 0x8b, 0xd9};
    if (!wait_for_runtime_code(base, set_detected_profile_rva, expected,
                               sizeof(expected))) {
        log_line("ERROR: controller input lock target did not decrypt in time.");
        return false;
    }
    if (!initialize_minhook()) {
        log_line("ERROR: MinHook initialization failed for controller input lock.");
        return false;
    }
    auto* target = reinterpret_cast<void*>(base + set_detected_profile_rva);
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the controller input lock hook.");
        return false;
    }
    const bool okay = create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_set_detected_profile),
        reinterpret_cast<void**>(&g_original_set_detected_profile),
        "controller input profile lock");
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (okay)
        log_line("Controller input lock installed at the detected-profile setter.");
    return okay;
}

static bool install_resolution_hook(std::uintptr_t base) {
    constexpr std::uintptr_t resolution_setter_rva = 0x65f050;
    constexpr unsigned char expected[] =
        {0x48, 0x89, 0x5c, 0x24, 0x18, 0x48, 0x89, 0x6c, 0x24, 0x20};
    auto* target = reinterpret_cast<unsigned char*>(base + resolution_setter_rva);
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        if (std::memcmp(target, expected, sizeof(expected)) == 0) break;
        if (attempt == 199) {
            log_line("ERROR: resolution setter did not decrypt in time.");
            return false;
        }
        Sleep(25);
    }

    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the resolution setter hook.");
        return false;
    }
    const bool initialized = initialize_minhook();
    const MH_STATUS create = initialized
        ? MH_CreateHook(target, reinterpret_cast<void*>(&hooked_set_resolution),
                        reinterpret_cast<void**>(&g_original_set_resolution))
        : MH_UNKNOWN;
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (!initialized || create != MH_OK || enable != MH_OK) {
        char message[256] = {};
        std::snprintf(message, sizeof(message),
                      "ERROR resolution hook: create=%s enable=%s target=%p",
                      MH_StatusToString(create), MH_StatusToString(enable), target);
        log_line(message);
        return false;
    }
    log_line("Resolution setter hook installed; no periodic polling is used.");
    return true;
}

static bool install_engine_hook(std::uintptr_t base) {
    constexpr std::uintptr_t projection_setter_rva = 0x0e3410;
    auto* target = reinterpret_cast<unsigned char*>(base + projection_setter_rva);

    // The protected executable is decrypted in memory. Wait for the known
    // prologue instead of asking MinHook to decode encrypted bytes.
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        if (target[0] == 0x48 && target[1] == 0x83 && target[2] == 0xec && target[3] == 0x68)
            break;
        if (attempt == 199) {
            log_line("ERROR: projection function did not decrypt in time.");
            return false;
        }
        Sleep(25);
    }

    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not temporarily make the projection page executable.");
        return false;
    }

    const bool initialized = initialize_minhook();
    const MH_STATUS create = initialized
        ? MH_CreateHook(target, reinterpret_cast<void*>(&hooked_set_projection),
                        reinterpret_cast<void**>(&g_original_set_projection))
        : MH_UNKNOWN;
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (!initialized || create != MH_OK || enable != MH_OK) {
        char message[256] = {};
        std::snprintf(message, sizeof(message),
                      "ERROR engine hook: create=%s enable=%s target=%p",
                      MH_StatusToString(create), MH_StatusToString(enable), target);
        log_line(message);
        return false;
    }
    log_line("Engine projection hook installed (DX11/DX12); Hor+ active.");
    return true;
}

static bool install_native_camera_fov_hook(std::uintptr_t base) {
    constexpr std::uintptr_t camera_builder_rva = 0x0b9bb0;
    constexpr unsigned char expected[] =
        {0x48, 0x8b, 0xc4, 0x53, 0x56, 0x57};
    auto* target = reinterpret_cast<unsigned char*>(base + camera_builder_rva);
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        if (std::memcmp(target, expected, sizeof(expected)) == 0) break;
        if (attempt == 199) {
            log_line("ERROR: native camera builder did not decrypt in time; renderer FOV fallback remains active.");
            return false;
        }
        Sleep(25);
    }

    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the native camera-FOV hook; renderer FOV fallback remains active.");
        return false;
    }
    const bool initialized = initialize_minhook();
    const MH_STATUS create = initialized
        ? MH_CreateHook(target, reinterpret_cast<void*>(&hooked_build_camera),
                        reinterpret_cast<void**>(&g_original_build_camera))
        : MH_UNKNOWN;
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (!initialized || create != MH_OK || enable != MH_OK) {
        char message[256] = {};
        std::snprintf(message, sizeof(message),
                      "ERROR native camera-FOV hook: create=%s enable=%s target=%p; renderer fallback remains active.",
                      MH_StatusToString(create), MH_StatusToString(enable), target);
        log_line(message);
        return false;
    }
    g_native_camera_fov_active = g_native_camera_fov_requested;
    if (g_native_camera_fov_active) {
        log_line("Native camera-FOV hook installed: input scale adjusted before engine projection, combined-matrix and frustum construction.");
    } else {
        log_line("Camera-ownership diagnostic hook installed in passive mode; native camera input is unchanged and renderer FOV fallback remains active.");
    }
    return true;
}

static bool install_cinematic_camera_owner_hook(std::uintptr_t base) {
    constexpr std::uintptr_t cinematic_camera_owner_rva = 0x652e00;
    constexpr unsigned char expected[] =
        {0x40, 0x55, 0x53, 0x57, 0x41, 0x56, 0x48, 0x8d};
    if (!wait_for_runtime_code(base, cinematic_camera_owner_rva, expected,
                               sizeof(expected))) {
        log_line("ERROR: cinematic camera owner did not decrypt in time; route 03 gameplay FOV remains active.");
        return false;
    }
    auto* target = reinterpret_cast<void*>(base + cinematic_camera_owner_rva);
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the cinematic camera owner hook.");
        return false;
    }
    const bool okay = create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_update_cinematic_camera),
        reinterpret_cast<void**>(&g_original_update_cinematic_camera),
        "cinematic camera owner");
    DWORD ignored = 0;
    VirtualProtect(target, 32, old_protection, &ignored);
    if (okay) {
        log_line("Cinematic camera owner hook installed: route 02 receives native FOV only inside FUN_140652e00.");
    }
    return okay;
}

static void put32(std::uintptr_t base, std::uintptr_t rva, std::uint32_t value) {
    *reinterpret_cast<volatile std::uint32_t*>(base + rva) = value;
}

static void put_resolution_pair_atomic(std::uintptr_t base, std::uintptr_t rva,
                                       std::uint32_t width, std::uint32_t height) {
    const LONG64 packed = static_cast<LONG64>(
        (static_cast<std::uint64_t>(height) << 32) | width);
    InterlockedExchange64(reinterpret_cast<volatile LONG64*>(base + rva), packed);
}

static void apply_resolution_state() {
    const auto base = g_executable_base;
    const auto width = g_target_width;
    const auto height = g_target_height;
    if (!base) return;

    put_resolution_pair_atomic(base, 0x1b00000, width, height);
    put_resolution_pair_atomic(base, 0x22a8d40, width, height);
    put_resolution_pair_atomic(base, 0x22a8d48, width, height);
    put32(base, 0x1ddda94, width); put32(base, 0x1ddda98, height);
    put32(base, 0x1dddaac, width); put32(base, 0x1dddab0, height);
    put_resolution_pair_atomic(base, 0x3bd1158, width, height);
    put_resolution_pair_atomic(base, 0x3bd1160, width, height);
    put_resolution_pair_atomic(base, 0x3bd1168, width, height);
    put_resolution_pair_atomic(base, 0x3bd1170, 0, 0);
    put_resolution_pair_atomic(base, 0x3bd1178, width, height);
    if (g_target_fps == 30 || g_target_fps == 60 || g_target_fps == 120)
        put32(base, 0x1b08df4, g_target_fps);
}

static DWORD WINAPI patch_thread(void*) {
    char ini_path[MAX_PATH] = {};
    if (!game_sibling_path(ini_path, settings_filename())) return 0;
#ifdef MGS4_LAB_ONLY
    if (GetPrivateProfileIntA("Lab", "Enabled", 0, ini_path) == 0) return 0;
#endif
    const std::uint32_t width = GetPrivateProfileIntA("Ultrawide", "Width", 3440, ini_path);
    const std::uint32_t height = GetPrivateProfileIntA("Ultrawide", "Height", 1440, ini_path);
    const std::uint32_t fps = GetPrivateProfileIntA("Ultrawide", "FPSLimit", 60, ini_path);
    char fov_text[32] = {};
    GetPrivateProfileStringA("Ultrawide", "FOVMultiplier", "1.000", fov_text,
                             sizeof(fov_text), ini_path);
    char* fov_end = nullptr;
    const float fov_multiplier = std::strtof(fov_text, &fov_end);
    char launcher_language[16] = {};
    GetPrivateProfileStringA("Launcher", "Language", "en", launcher_language,
                             sizeof(launcher_language), ini_path);
    g_constrain_ui = GetPrivateProfileIntA("Ultrawide", "ConstrainUITo16x9", 0,
                                           ini_path) != 0;
    g_anchor_ui = GetPrivateProfileIntA("Ultrawide", "AnchorUIToSafeArea", 0,
                                        ini_path) != 0;
    g_center_hud_16x9 = GetPrivateProfileIntA(
        "Ultrawide", "CenterHUDIn16x9", 0, ini_path) != 0;
    g_ui_diagnostics = GetPrivateProfileIntA("Ultrawide", "UIDiagnostics", 0,
                                              ini_path) != 0;
    g_cinematic_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "CinematicDiagnostics", 0, ini_path) != 0;
    g_projection_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "ProjectionDiagnostics", 0, ini_path) != 0;
    g_crosshair_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "CrosshairDiagnostics", 0, ini_path) != 0;
    g_camera_ownership_diagnostics = GetPrivateProfileIntA(
        "Ultrawide", "CameraOwnershipDiagnostics", 0, ini_path) != 0;
    g_camera_route_test_enabled = GetPrivateProfileIntA(
        "Ultrawide", "CameraRouteTest", 0, ini_path) != 0;
    if (g_camera_route_test_enabled) g_camera_ownership_diagnostics = true;
    g_native_camera_fov_requested = GetPrivateProfileIntA(
        "Ultrawide", "NativeCameraFOV", 1, ini_path) != 0;
    const bool xbox_prompts = GetPrivateProfileIntA("Input", "ForceXboxPrompts", 0, ini_path) != 0;
    const int legacy_force_xbox = GetPrivateProfileIntA(
        "Input", "ForceXboxInput", 0, ini_path);
    g_lock_controller_input = GetPrivateProfileIntA(
        "Input", "LockControllerInput", legacy_force_xbox, ini_path) != 0;
    g_input_wakeup_hz = GetPrivateProfileIntA("Input", "InputWakeupHz", 0, ini_path);
    g_input_diagnostics = GetPrivateProfileIntA("Input", "InputDiagnostics", 0,
                                                ini_path) != 0;
    if (!width || !height || fov_end == fov_text || !std::isfinite(fov_multiplier) ||
        fov_multiplier < 0.5f || fov_multiplier > 2.0f ||
        (g_input_wakeup_hz && (g_input_wakeup_hz < 10 || g_input_wakeup_hz > 1000))) {
        log_line("ERROR: invalid resolution, FOVMultiplier, or InputWakeupHz.");
        return 0;
    }
    g_target_width = width;
    g_target_height = height;
    g_target_fps = fps;
    g_fov_multiplier = fov_multiplier;
    g_target_aspect = static_cast<float>(width) / static_cast<float>(height);

    char settings_message[384] = {};
    std::snprintf(settings_message, sizeof(settings_message),
                  "Configuration: %ux%u, aspect %.6f, FOVMultiplier %.3f, FPS %u, "
                  "UI safe area %s, anchored UI %s, centered 16:9 HUD %s, "
                  "UI diagnostics %s, "
                  "cinematic diagnostics %s, projection diagnostics %s, "
                  "crosshair diagnostics %s, camera ownership diagnostics %s, "
                  "camera route test %s.",
                  width, height, g_target_aspect, g_fov_multiplier, fps,
                  g_constrain_ui ? "experimental" : "off",
                  g_anchor_ui ? "experimental" : "off",
                  g_center_hud_16x9 ? "EXPERIMENTAL/ACTIVE" : "off",
                  g_ui_diagnostics ? "on" : "off",
                  g_cinematic_diagnostics ? "on" : "off",
                  g_projection_diagnostics ? "on" : "off",
                  g_crosshair_diagnostics ? "on" : "off",
                  g_camera_ownership_diagnostics ? "on" : "off",
                  g_camera_route_test_enabled ? "diagnostic/active" : "off");
    log_line(settings_message);

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!supported_executable(base)) {
        log_line("ERROR: unrecognized mgs4.exe version; no offsets were applied.");
        return 0;
    }
    g_executable_base = base;
#ifdef MGS4_LAB_ONLY
    if (g_ui_diagnostics || g_crosshair_diagnostics)
        install_ui_common_diagnostics(base);
    if (g_crosshair_diagnostics)
        install_player_sight_diagnostics(base);
#ifdef MGS4_LAB_VERSION
    log_line("Experimental centered-HUD build: " MGS4_LAB_VERSION);
#endif
    log_line("Centered-HUD buffer mirrors: lifecycle-safe adaptive full-resource cache v2 active.");
    log_line("Centered-HUD companion: UI-only ASI active; resolution, FOV, FPS, input and launcher state remain owned by MGS4Ultra120.asi.");
    return 0;
#else
    if (g_camera_route_test_enabled) {
        std::strncpy(g_camera_route_control_path, ini_path,
                     sizeof(g_camera_route_control_path) - 1);
        if (char* slash = std::strrchr(g_camera_route_control_path, '\\')) {
            std::strcpy(slash + 1, "mgs4_fov_route_test.ini");
        }
        HANDLE route_thread = CreateThread(
            nullptr, 0, camera_route_control_thread, nullptr, 0, nullptr);
        if (route_thread) {
            CloseHandle(route_thread);
        } else {
            log_line("ERROR: camera route control thread could not start.");
        }
    }
    if (g_ui_diagnostics || g_crosshair_diagnostics)
        install_ui_common_diagnostics(base);
    if (g_cinematic_diagnostics)
        install_cinematic_diagnostics(base);
    if (g_crosshair_diagnostics)
        install_player_sight_diagnostics(base);
    if (g_input_diagnostics) install_xinput_diagnostics();
    if (g_input_diagnostics || g_input_wakeup_hz) {
        HANDLE input_thread = CreateThread(nullptr, 0, input_diagnostic_thread,
                                           nullptr, 0, nullptr);
        if (input_thread) CloseHandle(input_thread);
        char input_message[128] = {};
        std::snprintf(input_message, sizeof(input_message),
                      "Input experiment: diagnostics=%s, WM_NULL wakeup=%u Hz.",
                      g_input_diagnostics ? "on" : "off", g_input_wakeup_hz);
        log_line(input_message);
    }
    force_resolution_getters(base, width, height);
    install_resolution_hook(base);
    if (xbox_prompts) force_xbox_prompts(base);
    if (g_native_camera_fov_requested || g_camera_ownership_diagnostics ||
        g_camera_route_test_enabled) {
        const bool native_hook = install_native_camera_fov_hook(base);
        if (native_hook && g_native_camera_fov_requested &&
            !g_camera_route_test_enabled) {
            install_cinematic_camera_owner_hook(base);
        }
    }
    install_engine_hook(base);
    if (g_lock_controller_input)
        install_controller_input_lock(base);
    if (g_input_diagnostics)
        install_gamepad_pipeline_diagnostics(base);

    apply_resolution_state();
    // The display-mode hook handles subsequent changes.  A single delayed FPS
    // write happens after settings initialization; resolution is not polled.
    Sleep(2000);
    if (fps == 30 || fps == 60 || fps == 120) put32(base, 0x1b08df4, fps);
    const auto parsed_language = *reinterpret_cast<volatile LONG*>(
        base + 0x1b00428);
    char language_message[192] = {};
    std::snprintf(language_message, sizeof(language_message),
                  "Language diagnostic: configured token=%s, game parser id=%ld "
                  "(Spanish=5; this value is command-line owned and is not overwritten by MGS4SYS.SAV).",
                  launcher_language, parsed_language);
    log_line(language_message);
    char projection_message[224] = {};
    std::snprintf(projection_message, sizeof(projection_message),
                  "Projection activity after startup: native camera scales=%ld, renderer aspect corrections=%ld, native camera mode=%s.",
                  InterlockedCompareExchange(&g_native_camera_fov_patches, 0, 0),
                  InterlockedCompareExchange(&g_projection_patches, 0, 0),
                  g_native_camera_fov_active ? "active" : "fallback");
    log_line(projection_message);
    log_line("Initial state applied; patch thread finished without a polling loop.");
    return 0;
#endif
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void*) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE render_thread = CreateThread(nullptr, 0, render_backend_hook_thread,
                                            nullptr, 0, nullptr);
        if (render_thread) CloseHandle(render_thread);
        HANDLE thread = CreateThread(nullptr, 0, patch_thread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
