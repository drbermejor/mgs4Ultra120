#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#endif

#include <array>

#include "MinHook.h"
#include "camera_route_policy.h"
#include "game_profile.h"
#include "patch_resolver.h"
#include "projection_math.h"
#include "supersampling_math.h"
#include "reticle_truncation_patch.h"

#ifndef MGS4ULTRA120_VERSION
#define MGS4ULTRA120_VERSION "development"
#endif

#if defined(MGS4ULTRA120_WINMM_PROXY)
extern "C" FARPROC winmm_proxy_resolve_by_name(const char* name);
#endif
// Runtime state is written once by patch_thread before hooks are published.
// Hook callbacks then treat it as immutable, except for atomic activity
// counters and the controller-profile latch. Resolution changes update only
// the game's fields through apply_resolution_state().
static float g_target_aspect = 43.0f / 18.0f;
static volatile LONG g_projection_patches;
static volatile LONG g_extended_projection_patches;
static volatile LONG g_native_camera_fov_patches;
static std::uintptr_t g_executable_base;
static std::uint32_t g_output_width = 3440;
static std::uint32_t g_output_height = 1440;
static std::uint32_t g_target_width = 3440;
static std::uint32_t g_target_height = 1440;
static float g_fov_multiplier = 1.20f;
static float g_cinematic_fov_multiplier = 1.20f;
static float g_render_scale = 1.0f;
static bool g_enable_ultrawide = true;
static bool g_enable_resolution_override = true;
static bool g_controller_profile_fix;
static bool g_native_camera_fov_requested;
static bool g_native_camera_fov_active;
static bool g_experimental_cinematic_fov_requested;
static bool g_experimental_cinematic_fov_active;
static bool g_signature_audit;
static bool g_known_executable_profile;
static volatile LONG g_locked_controller_profile;
static volatile LONG g_minhook_state;
static volatile LONG g_resolved_groups;
static volatile LONG g_unresolved_groups;
// Addresses resolved once by patch_thread before the hooks that use them are
// published. Hook callbacks only read them.
static mgs4::resolver::Locator g_locator;
static mgs4::resolver::ResolutionTargets g_resolution;
static mgs4::resolver::CameraTargets g_camera;
static std::uintptr_t g_render_extent;
static std::uintptr_t g_controller_mask;
static std::array<std::uintptr_t, 4> g_reticle_sites;
static thread_local unsigned g_cinematic_camera_owner_depth;
static thread_local void* g_cinematic_camera_object;
static thread_local ULONGLONG g_cinematic_camera_tick;
static thread_local float g_cinematic_camera_input_scale;

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
static SetProjectionFn g_original_set_projection;
static BuildCameraFn g_original_build_camera;
static UpdateCinematicCameraFn g_original_update_cinematic_camera;
static SetResolutionFn g_original_set_resolution;
using SetDetectedProfileFn = void (__fastcall*)(std::int32_t);
static SetDetectedProfileFn g_original_set_detected_profile;

static void apply_resolution_state();
static bool initialize_minhook();
static void log_line(const char* message);

struct LargestProcessWindow {
    DWORD process_id;
    HWND window;
    std::uint64_t client_area;
};

static BOOL CALLBACK find_largest_process_window(HWND window, LPARAM parameter) {
    auto* result = reinterpret_cast<LargestProcessWindow*>(parameter);
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if (process_id != result->process_id || !IsWindowVisible(window) ||
        GetWindow(window, GW_OWNER)) return TRUE;
    RECT client = {};
    if (!GetClientRect(window, &client)) return TRUE;
    const auto width = static_cast<std::uint64_t>(client.right - client.left);
    const auto height = static_cast<std::uint64_t>(client.bottom - client.top);
    const auto area = width * height;
    if (area > result->client_area) {
        result->window = window;
        result->client_area = area;
    }
    return TRUE;
}

static void log_presentation_window_size() {
    LargestProcessWindow result = { GetCurrentProcessId(), nullptr, 0 };
    EnumWindows(find_largest_process_window,
                reinterpret_cast<LPARAM>(&result));
    if (!result.window) {
        log_line("WARNING: no visible top-level game window was found for output-size verification.");
        return;
    }
    RECT client = {};
    RECT outer = {};
    GetClientRect(result.window, &client);
    GetWindowRect(result.window, &outer);
    char message[256] = {};
    std::snprintf(message, sizeof(message),
                  "Presentation window after startup: client=%ldx%ld, outer=%ldx%ld, DPI=%u; configured output=%ux%u, internal render=%ux%u.",
                  client.right - client.left, client.bottom - client.top,
                  outer.right - outer.left, outer.bottom - outer.top,
                  GetDpiForWindow(result.window), g_output_width,
                  g_output_height, g_target_width, g_target_height);
    log_line(message);
}

// The protected executable decrypts code before changing the page from RW to
// RX. On native Windows a signature can therefore be visible while
// VirtualProtect still reports PAGE_READWRITE. Restoring that transient value
// after installing a hook makes the first jump back into the game fault with
// STATUS_ACCESS_VIOLATION (execute). Proton tolerated this timing, which hid
// the bug during the original Linux validation. Once code has been patched or
// hooked, keep it executable and read-only unless it already had a stricter
// executable protection.
static DWORD final_code_protection(DWORD previous) {
    const DWORD base = previous & 0xff;
    if (base == PAGE_EXECUTE || base == PAGE_EXECUTE_READ ||
        base == PAGE_EXECUTE_READWRITE || base == PAGE_EXECUTE_WRITECOPY)
        return previous;
    return PAGE_EXECUTE_READ;
}

// Parse the small decimal format used by the INI without consulting the
// process locale. The game may change the C locale, which made the old strtof
// path interpret a perfectly valid "1.000" differently on some Windows
// installations. Accept both decimal separators for hand-edited files.
static bool parse_ini_decimal(const char* text, float* value) {
    if (!text || !value) return false;
    while (*text == ' ' || *text == '\t') ++text;
    bool negative = false;
    if (*text == '+' || *text == '-') {
        negative = *text == '-';
        ++text;
    }
    double result = 0.0;
    bool have_digit = false;
    while (*text >= '0' && *text <= '9') {
        have_digit = true;
        result = result * 10.0 + (*text++ - '0');
    }
    if (*text == '.' || *text == ',') {
        ++text;
        double place = 0.1;
        while (*text >= '0' && *text <= '9') {
            have_digit = true;
            result += (*text++ - '0') * place;
            place *= 0.1;
        }
    }
    while (*text == ' ' || *text == '\t') ++text;
    if (!have_digit || *text != '\0') return false;
    if (negative) result = -result;
    *value = static_cast<float>(result);
    return std::isfinite(*value);
}

static bool ascii_equals_ignore_case(const char* left, const char* right) {
    if (!left || !right) return false;
    while (*left && *right) {
        char a = *left++;
        char b = *right++;
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return *left == '\0' && *right == '\0';
}

// The PC port can spuriously select profile 0 (keyboard) while an XInput slot
// remains connected. Its subsequent keyboard merge then neutralizes otherwise
// valid controller axes. Preserve whichever native controller family the game
// detected (profiles 1..7) until every controller slot is disconnected. This
// neither synthesizes input nor periodically rewrites game state.
static void __fastcall hooked_set_detected_profile(std::int32_t profile) {
    if (g_controller_profile_fix && g_controller_mask) {
        const LONG connected_mask =
            *reinterpret_cast<volatile LONG*>(g_controller_mask);
        if (!connected_mask) {
            InterlockedExchange(&g_locked_controller_profile, 0);
        } else if (profile >= 1 && profile <= 7) {
            InterlockedExchange(&g_locked_controller_profile, profile);
        } else if (profile == 0) {
            const LONG locked = InterlockedCompareExchange(
                &g_locked_controller_profile, 0, 0);
            if (locked >= 1 && locked <= 7) profile = locked;
        }
    }
    g_original_set_detected_profile(profile);
}

static void log_line(const char* message) {
    char module_path[MAX_PATH] = {};
    // Store the log and INI beside mgs4.exe for both distribution layouts.
    // The ASI itself lives under scripts, while the legacy alpha.3 proxy lived
    // in the executable directory.
    GetModuleFileNameA(nullptr, module_path, MAX_PATH);
    char* slash = std::strrchr(module_path, '\\');
    if (slash) std::strcpy(slash + 1, "mgs4_ultrawide.log");
    HANDLE file = CreateFileA(module_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, message, static_cast<DWORD>(std::strlen(message)), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

static bool create_and_enable_hook(void* target, void* detour, void** original,
                                   const char* name) {
    // Callers must first validate the supported executable, wait for the
    // protected target to decrypt, and temporarily grant execute/write access.
    // Keeping those checks outside this helper makes each hook's expected
    // bytes visible beside its RVA.
    const MH_STATUS create = MH_CreateHook(target, detour, original);
    const MH_STATUS enable =
        create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    if (create == MH_OK && enable == MH_OK) return true;
    char message[256] = {};
    std::snprintf(message, sizeof(message),
                  "ERROR hook %s: create=%s enable=%s", name,
                  MH_StatusToString(create), MH_StatusToString(enable));
    log_line(message);
    return false;
}

// Resolve one target group. The loader can start this thread before Steam's
// protected code is decrypted, so a group is retried during a bounded startup
// window. In relocation mode each attempt scans .text, so attempts are spaced
// further apart. A group is used only when every address in it resolved and
// its cross-checks agreed; otherwise its feature is not installed.
template <typename Resolve>
static bool wait_for_group(const char* name, Resolve resolve) {
    const bool scanning =
        g_locator.mode == mgs4::resolver::Mode::relocate;
    const unsigned attempts = scanning ? 100 : 200;
    const DWORD delay = scanning ? 100 : 25;
    for (unsigned attempt = 0; attempt < attempts; ++attempt) {
        if (resolve()) {
            InterlockedIncrement(&g_resolved_groups);
            if (scanning) {
                char message[256]{};
                std::snprintf(message, sizeof(message),
                              "Signature group resolved by unique .text matches: %s.",
                              name);
                log_line(message);
            }
            return true;
        }
        Sleep(delay);
    }
    InterlockedIncrement(&g_unresolved_groups);
    char message[256]{};
    std::snprintf(message, sizeof(message),
                  scanning
                      ? "ERROR: %s did not resolve to unique, consistent signatures; that feature was not installed."
                      : "ERROR: %s did not decrypt or match at the reference profile addresses; that feature was not installed.",
                  name);
    log_line(message);
    return false;
}

// Diagnostic only: count the .text matches of every core signature before any
// hook changes those bytes. The reference addresses stay in use.
static void audit_core_signatures() {
    const mgs4::resolver::Locator scan{g_locator.base, g_locator.text,
                                       mgs4::resolver::Mode::relocate};
    unsigned warnings = 0;
    for (const auto& entry : mgs4::windows::kCoreAuditWindows) {
        const std::size_t matches =
            mgs4::resolver::count_matches(scan, *entry.window);
        const bool okay = matches == entry.expected_matches;
        if (!okay) ++warnings;
        char message[256]{};
        std::snprintf(message, sizeof(message),
                      "%sSignature audit: %s has %zu .text match(es); expected %zu.",
                      okay ? "" : "WARNING: ", entry.window->name, matches,
                      entry.expected_matches);
        log_line(message);
    }
    char summary[128]{};
    std::snprintf(summary, sizeof(summary),
                  "Signature audit finished with %u warning(s).", warnings);
    log_line(summary);
}

#if defined(MGS4ULTRA120_WINMM_PROXY)
extern "C" MMRESULT WINAPI mgs4_timeBeginPeriod(UINT period) {
    auto fn = reinterpret_cast<TimeBeginPeriodFn>(
        winmm_proxy_resolve_by_name("timeBeginPeriod"));
    return fn ? fn(period) : TIMERR_NOERROR;
}

extern "C" DWORD WINAPI mgs4_timeGetTime() {
    auto fn = reinterpret_cast<TimeGetTimeFn>(
        winmm_proxy_resolve_by_name("timeGetTime"));
    return fn ? fn() : GetTickCount();
}
#endif

// Renderer-level projection correction used by the published alpha.5 binary.
// Depending on the engine path, this setter receives either a canonical 16:9
// matrix or one whose X scale already matches the output aspect. Both are
// original, unmodified camera states here. They receive configured FOV only as
// an automatic fallback when the requested native hook cannot start. An
// explicit NativeCameraFOV=0 keeps aspect correction but preserves vertical FOV.
//
// Do not move this rewrite back into the central camera builder without a new
// native-Windows visual gate. The first alpha.6 package did so and applied an
// additional horizontal transform later in the engine, making characters look
// unnaturally tall and thin even when supersampling was disabled. Close-up
// continuity is handled only here, by accepting structurally valid projections
// above the legacy m00/m11 ceilings; no camera/frustum state is rewritten.
static void __fastcall hooked_set_projection(const float* matrix) {
    if (!matrix) {
        g_original_set_projection(matrix);
        return;
    }

    float copy[16];
    std::memcpy(copy, matrix, sizeof(copy));
    const float original_x = copy[0];
    const float original_y = copy[5];
    bool exceeded_legacy_limits = false;
    const bool adjusted = mgs4_camera::renderer_is_aspect_only(
                              g_native_camera_fov_active,
                              g_native_camera_fov_requested)
        ? mgs4_projection::adjust_renderer_aspect_only(
              copy, g_target_aspect, &exceeded_legacy_limits)
        : mgs4_projection::adjust_renderer_projection(
              copy, g_target_aspect, g_fov_multiplier,
              &exceeded_legacy_limits);
    if (adjusted) {
        InterlockedIncrement(&g_projection_patches);
        if (exceeded_legacy_limits) {
            const LONG count = InterlockedIncrement(
                &g_extended_projection_patches);
            if (count == 1) {
                char message[256] = {};
                std::snprintf(message, sizeof(message),
                              "Common projection setter accepted the first projection beyond the legacy scale limits: m00=%.7f m11=%.7f aspect=%.7f; native camera FOV mode=%s.",
                              original_x, original_y,
                              std::fabs(original_y / original_x),
                              g_native_camera_fov_active ? "active" : "fallback");
                log_line(message);
            }
        }
    }
    g_original_set_projection(copy);
}

// Native input-level FOV hook. The original function remains solely
// responsible for creating its three projection variants, combined matrices
// and six normalized visibility planes. Unlike the withdrawn alpha.6 code,
// nothing in the camera object is rewritten after the original returns.
static void __fastcall hooked_build_camera(void* camera, const void* source,
                                           float projection_scale,
                                           float parameter4,
                                           float parameter5,
                                           float aspect_scale) {
#if defined(_MSC_VER)
    const auto return_address =
        reinterpret_cast<std::uintptr_t>(_ReturnAddress());
#else
    const auto return_address = reinterpret_cast<std::uintptr_t>(
        __builtin_return_address(0));
#endif
    // Map the resolved return address to its reference identity so the route
    // policy stays independent of where this build placed the call.
    const std::uintptr_t caller_return_rva =
        return_address == g_camera.primary_return
            ? mgs4_camera::kPrimaryFovReturnRva
        : return_address == g_camera.cinematic_source_return
            ? mgs4_camera::kCinematicSourceReturnRva
        : return_address == g_camera.cinematic_final_return
            ? mgs4_camera::kCinematicFinalRebuildReturnRva
            : 0;
    const bool direct_cinematic_owner =
        g_experimental_cinematic_fov_active &&
        g_cinematic_camera_owner_depth != 0 &&
        mgs4_camera::owns_cinematic_source(caller_return_rva);
    const ULONGLONG now_tick = GetTickCount64();
    const float propagation_tolerance =
        std::fmax(0.001f, std::fabs(g_cinematic_camera_input_scale) * 0.001f);
    const bool final_cinematic_rebuild =
        g_experimental_cinematic_fov_active &&
        mgs4_camera::owns_cinematic_final_rebuild(caller_return_rva) &&
        camera == g_cinematic_camera_object &&
        now_tick - g_cinematic_camera_tick <= 250 &&
        std::fabs(projection_scale - g_cinematic_camera_input_scale) <=
            propagation_tolerance;
    const bool gameplay_owner =
        mgs4_camera::owns_native_fov(caller_return_rva);
    const bool apply_native_fov = gameplay_owner || direct_cinematic_owner ||
                                  final_cinematic_rebuild;
    const float selected_multiplier =
        direct_cinematic_owner || final_cinematic_rebuild
            ? g_cinematic_fov_multiplier
            : g_fov_multiplier;
    const float adjusted_scale = apply_native_fov
        ? mgs4_projection::adjust_camera_input_scale(
              projection_scale, selected_multiplier)
        : projection_scale;
    if (direct_cinematic_owner) {
        g_cinematic_camera_object = camera;
        g_cinematic_camera_tick = now_tick;
        g_cinematic_camera_input_scale = projection_scale;
    }
    if (adjusted_scale != projection_scale)
        InterlockedIncrement(&g_native_camera_fov_patches);
    g_original_build_camera(camera, source, adjusted_scale, parameter4,
                            parameter5, aspect_scale);
}

// Runtime ownership tracing isolated FUN_140652e00 as the high-level owner of
// the tested in-engine cinematic camera. It enters the shared route-02 wrapper,
// so a TLS scope selects only this owner and leaves WeaponWindow/auxiliary
// cameras untouched. Route 06 is the validated final rebuild.
static void __fastcall hooked_update_cinematic_camera(void* context) {
    ++g_cinematic_camera_owner_depth;
    g_original_update_cinematic_camera(context);
    --g_cinematic_camera_owner_depth;
}

// Central display-mode setter. The launcher keeps the physical output/window
// size, while this engine path receives the internal render size. When
// supersampling is disabled both sizes are identical, preserving the stable
// release path exactly.
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
    // This PE tuple selects the reference profile: every signature window is
    // then required at its recorded RVA and no scan is needed.
    return mgs4::profile::matches_loaded_image(
        base, mgs4::profile::kSteam20260911);
}

// The game copies these two compositor getters into its global render size
// during startup.  The proxy thread used to race that initialization: the
// compositor was corrected afterwards, but the global could remain 2560x1440
// for the whole session.  Cinematic/post-processing passes then sampled a
// 2560-wide surface into a 3440-wide output, corrupting the extra right-hand
// region.  Replace the trivial getters before initialization can copy them.
static bool force_resolution_getters(std::uint32_t width,
                                     std::uint32_t height) {
    struct GetterPatch {
        std::uintptr_t address;
        std::uint32_t value;
    };
    const GetterPatch patches[] = {
        {g_resolution.width_getter, width},
        {g_resolution.height_getter, height},
    };

    for (const GetterPatch& patch : patches) {
        // Resolution already decoded both getters; re-check immediately before
        // writing so discovery and mutation stay separate.
        std::uintptr_t data{};
        if (!mgs4::resolver::decode_getter(g_locator, patch.address, &data)) {
            log_line("ERROR: a resolution getter changed before it could be written.");
            return false;
        }
        auto* target = reinterpret_cast<unsigned char*>(patch.address);
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
        VirtualProtect(target, sizeof(replacement),
                       final_code_protection(old_protection), &ignored);
    }
    log_line("Resolution getters fixed before surface initialization.");
    return true;
}

// The aiming reticle reaches the UI canvas through signed 16-bit truncations.
// `cvttss2si` produces each position in 1/16 px, and the following `movsx`
// instructions keep only the low 16 bits.  For the first X route:
//
//     0xe3a13c  cvttss2si ecx, xmm0     ; ecx = screen_x * 16
//     0xe3a146  movsx edx, cx           ; truncates to int16
//     0xe3a14d  lea eax,[rdx+rdx*4]
//     0xe3a150  shl eax, 8              ; x1280, the UI canvas width
//     0xe3a153  cdq
//     0xe3a154  idiv [render width]
//
// A centred reticle stores width/2 * 16, so the value crosses 32767 at exactly
// 4096 px of internal width: 2048*16 = 32768.  That is the boundary recorded in
// v0.3.1-alpha.6 as stable at 3956x1656 and flickering at 4096.  At 5120 wide,
// 2560*16 = 40960 wraps to -24576, placing the reticle at
// -24576*1280/5120 = -6144 in 1/16 canvas units, off the left edge.
//
// Replacing the truncation with a plain 32-bit move keeps the real value, so
// 40960*1280/5120 = 10240 - the canvas centre.  `mov edx, ecx` is one byte
// shorter than `movsx edx, cx`, so the third byte becomes a nop and no
// surrounding instruction moves.
//
// The second X route and both Y routes use the same lossy conversion.  At the
// screen centre, either axis reaches the signed limit when its internal extent
// reaches 4096 pixels.  Current guidance warns above 2x supersampling, so the Y
// limit is less likely at 1440p, but removing all four truncations avoids a
// latent axis-dependent ceiling.
static bool fix_reticle_truncation() {
    using mgs4_reticle::PatchSetState;
    using mgs4_reticle::TruncationPatch;
    // truncation_patches keeps the reference RVAs as identities; the resolved
    // sites are stored in the same X, Y, Y, X order.
    const auto site = [](const TruncationPatch& patch) {
        const auto index = static_cast<std::size_t>(
            &patch - mgs4_reticle::truncation_patches.data());
        return reinterpret_cast<unsigned char*>(g_reticle_sites[index]);
    };
    const auto read_site = [&site](const TruncationPatch& patch) {
        return static_cast<const unsigned char*>(site(patch));
    };

    PatchSetState state = PatchSetState::Unavailable;
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        state = mgs4_reticle::classify_patch_set(read_site);
        if (state == PatchSetState::Original ||
            state == PatchSetState::Applied ||
            state == PatchSetState::Mixed) break;
        Sleep(25);
    }
    if (state == PatchSetState::Applied) {
        log_line("Reticle 16-bit truncation was already removed on both axes.");
        return true;
    }
    if (state == PatchSetState::Mixed) {
        log_line("ERROR: reticle truncation sites are only partially patched; no additional bytes were written.");
        return false;
    }
    if (state != PatchSetState::Original) {
        log_line("WARNING: reticle truncation sites did not decrypt in time; no reticle bytes were changed.");
        return false;
    }

    unsigned char* patch_begin = nullptr;
    unsigned char* patch_end = nullptr;
    for (const TruncationPatch& patch : mgs4_reticle::truncation_patches) {
        unsigned char* begin = site(patch);
        if (!patch_begin || begin < patch_begin) patch_begin = begin;
        if (!patch_end || begin + patch.size > patch_end)
            patch_end = begin + patch.size;
    }
    const std::size_t patch_span =
        static_cast<std::size_t>(patch_end - patch_begin);
    DWORD old_protection = 0;
    if (!VirtualProtect(patch_begin, patch_span, PAGE_EXECUTE_READWRITE,
                        &old_protection)) {
        log_line("ERROR: could not write the reticle truncation sites.");
        return false;
    }
    for (const TruncationPatch& patch : mgs4_reticle::truncation_patches)
        std::memcpy(site(patch), patch.replacement.data(), patch.size);
    FlushInstructionCache(GetCurrentProcess(), patch_begin, patch_span);
    const bool verified =
        mgs4_reticle::classify_patch_set(read_site) == PatchSetState::Applied;
    DWORD ignored = 0;
    const bool protection_restored = VirtualProtect(
        patch_begin, patch_span, final_code_protection(old_protection), &ignored) != 0;
    if (!verified) {
        log_line("ERROR: reticle truncation patch did not verify after writing.");
        return false;
    }
    if (!protection_restored)
        log_line("WARNING: reticle truncation patch could not restore the original page protection.");
    log_line("Reticle 16-bit truncation removed on X and Y; the four patched routes keep full 32-bit coordinates.");
    return protection_restored;
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

static bool install_controller_profile_fix() {
    mgs4::resolver::ControllerTargets controller{};
    if (!wait_for_group("controller-profile setter and connection mask",
                        [&] {
                            return mgs4::resolver::resolve_controller(
                                g_locator, &controller);
                        }))
        return false;
    g_controller_mask = controller.connection_mask;
    auto* target = reinterpret_cast<unsigned char*>(controller.setter);

    if (!initialize_minhook()) {
        log_line("ERROR: MinHook initialization failed for controller-profile fix.");
        return false;
    }
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the controller-profile fix hook.");
        return false;
    }
    const bool okay = create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_set_detected_profile),
        reinterpret_cast<void**>(&g_original_set_detected_profile),
        "controller profile fix");
    DWORD ignored = 0;
    VirtualProtect(target, 32,
                   okay ? final_code_protection(old_protection) : old_protection,
                   &ignored);
    if (okay)
        log_line("Controller-profile fix installed; connected pad family is preserved.");
    return okay;
}

static bool install_resolution_hook() {
    auto* target = reinterpret_cast<unsigned char*>(g_resolution.setter);

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
    const bool okay = initialized && create == MH_OK && enable == MH_OK;
    VirtualProtect(target, 32,
                   okay ? final_code_protection(old_protection) : old_protection,
                   &ignored);
    if (!okay) {
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

static bool install_engine_hook() {
    std::uintptr_t setter{};
    if (!wait_for_group("projection setter", [&] {
            setter = mgs4::resolver::resolve_projection_setter(g_locator);
            return setter != 0;
        }))
        return false;
    auto* target = reinterpret_cast<unsigned char*>(setter);

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
    const bool okay = initialized && create == MH_OK && enable == MH_OK;
    VirtualProtect(target, 32,
                   okay ? final_code_protection(old_protection) : old_protection,
                   &ignored);
    if (!okay) {
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

static bool install_native_camera_fov_hook() {
    mgs4::resolver::CameraTargets camera{};
    if (!wait_for_group("native camera builder and caller routes", [&] {
            return mgs4::resolver::resolve_camera(g_locator, &camera);
        })) {
        log_line("Native camera builder unresolved; common-setter FOV fallback remains active.");
        return false;
    }
    g_camera = camera;
    auto* target = reinterpret_cast<unsigned char*>(camera.builder);

    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the native camera-FOV hook; common-setter FOV fallback remains active.");
        return false;
    }

    const bool initialized = initialize_minhook();
    const MH_STATUS create = initialized
        ? MH_CreateHook(target, reinterpret_cast<void*>(&hooked_build_camera),
                        reinterpret_cast<void**>(&g_original_build_camera))
        : MH_UNKNOWN;
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : MH_UNKNOWN;
    DWORD ignored = 0;
    const bool okay = initialized && create == MH_OK && enable == MH_OK;
    VirtualProtect(target, 32,
                   okay ? final_code_protection(old_protection) : old_protection,
                   &ignored);
    if (!okay) {
        char message[256] = {};
        std::snprintf(message, sizeof(message),
                      "ERROR native camera-FOV hook: create=%s enable=%s target=%p; common-setter fallback remains active.",
                      MH_StatusToString(create), MH_StatusToString(enable), target);
        log_line(message);
        return false;
    }
    g_native_camera_fov_active = true;
    log_line("Native camera-FOV hook installed: input scale is adjusted before the game builds projections, combined matrices and frustum planes.");
    return true;
}

static bool install_cinematic_camera_owner_hook() {
    std::uintptr_t owner{};
    if (!wait_for_group("experimental cinematic camera owner", [&] {
            owner = mgs4::resolver::resolve_cinematic_owner(g_locator);
            return owner != 0;
        })) {
        log_line("Experimental cinematic camera owner unresolved; cinematic FOV remains disabled.");
        return false;
    }
    auto* target = reinterpret_cast<unsigned char*>(owner);
    DWORD old_protection = 0;
    if (!VirtualProtect(target, 32, PAGE_EXECUTE_READWRITE, &old_protection)) {
        log_line("ERROR: could not enable the experimental cinematic camera hook.");
        return false;
    }
    const bool okay = initialize_minhook() && create_and_enable_hook(
        target, reinterpret_cast<void*>(&hooked_update_cinematic_camera),
        reinterpret_cast<void**>(&g_original_update_cinematic_camera),
        "experimental cinematic camera owner");
    DWORD ignored = 0;
    VirtualProtect(target, 32,
                   okay ? final_code_protection(old_protection) : old_protection,
                   &ignored);
    if (!okay) return false;
    g_experimental_cinematic_fov_active = true;
    log_line("Experimental cinematic FOV hook installed: scoped route 02 and final rebuild route 06 only.");
    return true;
}

static void put32(std::uintptr_t address, std::uint32_t value) {
    *reinterpret_cast<volatile std::uint32_t*>(address) = value;
}

static void put_resolution_pair_atomic(std::uintptr_t address,
                                       std::uint32_t width, std::uint32_t height) {
    // The game reads adjacent width/height fields as one state. Publish both in
    // a single aligned exchange so a render thread cannot observe mixed epochs.
    const LONG64 packed = static_cast<LONG64>(
        (static_cast<std::uint64_t>(height) << 32) | width);
    InterlockedExchange64(reinterpret_cast<volatile LONG64*>(address), packed);
}

static void apply_resolution_state() {
    // These are native render-state mirrors, not presentation-window fields.
    // The display-mode hook calls this after every game-owned mode change; the
    // startup thread calls it once for the already-created initial state.
    const auto& targets = g_resolution;
    const auto width = g_target_width;
    const auto height = g_target_height;
    if (!targets.render_extent) return;

    if (g_enable_resolution_override) {
        put_resolution_pair_atomic(targets.render_extent, width, height);
        put_resolution_pair_atomic(targets.scaled_extent, width, height);
        put_resolution_pair_atomic(targets.scaled_extent + 0x08, width, height);
        put32(targets.compositor_mirror + 0x00, width);
        put32(targets.compositor_mirror + 0x04, height);
        put32(targets.compositor_mirror + 0x18, width);
        put32(targets.compositor_mirror + 0x1c, height);
        put_resolution_pair_atomic(targets.getter_block + 0x00, width, height);
        put_resolution_pair_atomic(targets.getter_block + 0x08, width, height);
        put_resolution_pair_atomic(targets.getter_block + 0x10, width, height);
        put_resolution_pair_atomic(targets.getter_block + 0x18, 0, 0);
        put_resolution_pair_atomic(targets.getter_block + 0x20, width, height);
    }
}

static DWORD WINAPI patch_thread(void*) {
    // Initialization is deliberately phase-ordered:
    //   1. parse and validate configuration without touching game memory;
    //   2. gate the executable profile;
    //   3. install event-driven hooks/patches after protected code decrypts;
    //   4. publish the initial resolution state and finish.
    // No maintenance or per-frame polling thread remains after this returns.
    log_line("MGS4 Ultra120 " MGS4ULTRA120_VERSION);
#if defined(MGS4ULTRA120_ASI)
    log_line("Module layout: MGS4Ultra120.asi loaded by an external ASI loader.");
#else
    log_line("Module layout: combined MGS4 Ultra120 WinMM proxy.");
#endif
    char ini_path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, ini_path, MAX_PATH);
    if (char* slash = std::strrchr(ini_path, '\\'))
        std::strcpy(slash + 1, "mgs4_ultrawide.ini");
    const bool enable_ultrawide =
        GetPrivateProfileIntA("Patch", "UltrawideEnabled", 1, ini_path) != 0;
    const std::uint32_t width =
        GetPrivateProfileIntA("Ultrawide", "Width", 3440, ini_path);
    const std::uint32_t height =
        GetPrivateProfileIntA("Ultrawide", "Height", 1440, ini_path);
    const bool enable_supersampling = GetPrivateProfileIntA(
        "Supersampling", "SupersamplingEnabled", 0, ini_path) != 0;
    const bool allow_unsupported = GetPrivateProfileIntA(
        "Patch", "AllowUnsupportedExecutable", 0, ini_path) != 0;
    const bool controller_profile_fix = GetPrivateProfileIntA(
        "Input", "ControllerProfileFixEnabled", 0, ini_path) != 0;
    const bool signature_audit = GetPrivateProfileIntA(
        "Diagnostics", "SignatureAudit", 0, ini_path) != 0;
    const bool signature_relocation = GetPrivateProfileIntA(
        "Patch", "SignatureRelocation", 1, ini_path) != 0;
    const bool force_relocation = GetPrivateProfileIntA(
        "Diagnostics", "ForceSignatureRelocation", 0, ini_path) != 0;
    g_native_camera_fov_requested = GetPrivateProfileIntA(
        "Ultrawide", "NativeCameraFOV", 1, ini_path) != 0;
    g_experimental_cinematic_fov_requested = GetPrivateProfileIntA(
        "Ultrawide", "ExperimentalCinematicFOV", 0, ini_path) != 0;
    char fov_text[32] = {};
    GetPrivateProfileStringA("Ultrawide", "FOVMultiplier", "1.200", fov_text,
                             sizeof(fov_text), ini_path);
    float fov_multiplier = 1.20f;
    const bool fov_valid = parse_ini_decimal(fov_text, &fov_multiplier);
    char cinematic_fov_text[32] = {};
    GetPrivateProfileStringA("Ultrawide", "CinematicFOVMultiplier", "inherit",
                             cinematic_fov_text,
                             sizeof(cinematic_fov_text), ini_path);
    const bool cinematic_fov_inherits =
        ascii_equals_ignore_case(cinematic_fov_text, "inherit");
    float cinematic_fov_multiplier = fov_multiplier;
    const bool cinematic_fov_valid = cinematic_fov_inherits ||
        parse_ini_decimal(cinematic_fov_text, &cinematic_fov_multiplier);
    char render_scale_text[32] = {};
    GetPrivateProfileStringA("Supersampling", "RenderScale", "1.50",
                             render_scale_text, sizeof(render_scale_text),
                             ini_path);
    float render_scale = 1.0f;
    const bool render_scale_valid =
        parse_ini_decimal(render_scale_text, &render_scale);
    std::uint32_t render_width = width;
    std::uint32_t render_height = height;
    const bool render_extent_valid = !enable_supersampling ||
        (render_scale_valid && mgs4_supersampling::compute_render_extent(
            width, height, render_scale, &render_width, &render_height));
    if (!width || !height ||
        (enable_ultrawide && (!fov_valid || fov_multiplier < 0.5f)) ||
        (enable_ultrawide && g_experimental_cinematic_fov_requested &&
         (!cinematic_fov_valid || cinematic_fov_multiplier < 0.5f)) ||
        !render_extent_valid) {
        char invalid_message[512] = {};
        std::snprintf(invalid_message, sizeof(invalid_message),
                      "ERROR: invalid display configuration in %s: output=%ux%u, FOVMultiplier='%s', CinematicFOVMultiplier='%s' (use 'inherit' or a finite value of at least 0.500), SupersamplingEnabled=%u, RenderScale='%s' (must be finite, at least 1.0, and fit the game's 32-bit resolution fields).",
                      ini_path, width, height, fov_text,
                      cinematic_fov_text,
                      enable_supersampling ? 1u : 0u, render_scale_text);
        log_line(invalid_message);
        return 0;
    }
    g_enable_ultrawide = enable_ultrawide;
    g_enable_resolution_override = enable_ultrawide || enable_supersampling;
    g_controller_profile_fix = controller_profile_fix;
    g_signature_audit = signature_audit;
    g_output_width = width;
    g_output_height = height;
    g_target_width = render_width;
    g_target_height = render_height;
    g_fov_multiplier = fov_multiplier;
    g_cinematic_fov_multiplier = cinematic_fov_inherits
        ? fov_multiplier : cinematic_fov_multiplier;
    g_render_scale = enable_supersampling ? render_scale : 1.0f;
    g_target_aspect = static_cast<float>(width) / static_cast<float>(height);

    char settings_message[512] = {};
    std::snprintf(settings_message, sizeof(settings_message),
                  "Configuration: output %ux%u; internal render %ux%u; supersampling %s (scale %.3f); ultrawide/FOV %s (aspect %.6f, gameplay FOV %.3f, NativeCameraFOV requested=%s); experimental cinematic FOV requested=%s (multiplier %.3f, %s); controller-profile fix %s. FPS timing is delegated to MGSFPSUnlock.",
                  g_output_width, g_output_height, g_target_width,
                  g_target_height, enable_supersampling ? "on" : "off",
                  g_render_scale, enable_ultrawide ? "on" : "off",
                  g_target_aspect, g_fov_multiplier,
                  g_native_camera_fov_requested ? "yes" : "no",
                  g_experimental_cinematic_fov_requested ? "yes" : "no",
                  g_cinematic_fov_multiplier,
                  cinematic_fov_inherits ? "inherits gameplay" : "separate",
                  controller_profile_fix ? "on" : "off");
    log_line(settings_message);
    if (enable_ultrawide && fov_multiplier > 1.20f) {
        log_line("WARNING: FOVMultiplier exceeds the tested 1.200 recommendation. No upper limit is enforced; unusual framing and early edge-of-frame geometry or animation visibility are the user's responsibility.");
    }
    if (enable_ultrawide && g_experimental_cinematic_fov_requested) {
        log_line("WARNING: cinematic FOV is an opt-in preview. Expanded framing can reveal characters, objects, geometry or animation transitions before the authored shot intended them to enter the frame. This expected scene pop-in/early visibility is distinct from the old projection/frustum culling regression.");
        if (g_cinematic_fov_multiplier > 1.20f) {
            log_line("WARNING: CinematicFOVMultiplier exceeds the 1.200 preview recommendation and has not been broadly validated.");
        }
    }
    if (enable_supersampling && (render_scale > 2.0f ||
        render_width > 16384 || render_height > 16384)) {
        log_line("WARNING: experimental supersampling exceeds the conservative 2x/16384-pixel guidance. No GPU/VRAM capacity limit is enforced; performance, stability and driver behavior are the user's responsibility.");
    }
    if (enable_supersampling && render_scale == 1.0f)
        log_line("WARNING: supersampling is enabled at 1.0x, so internal and output resolution are identical.");

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    g_known_executable_profile = supported_executable(base);
    mgs4::resolver::Mode mode = mgs4::resolver::Mode::reference;
    if (!g_known_executable_profile) {
        if (!signature_relocation && !allow_unsupported) {
            log_line("ERROR: unrecognized mgs4.exe version and SignatureRelocation=0; no changes were applied.");
            return 0;
        }
        // An unknown build is patched only where every signature of a feature
        // group is found exactly once and its native relationships agree.
        mode = mgs4::resolver::Mode::relocate;
        log_line("WARNING: unrecognized mgs4.exe version. This build has not been validated; each feature is installed only if all of its signatures resolve uniquely and pass their cross-checks. Set SignatureRelocation=0 to apply nothing instead.");
    } else if (force_relocation) {
        // Diagnostic: exercise the unknown-build path on the reference build.
        mode = mgs4::resolver::Mode::relocate;
        log_line("Diagnostics: ForceSignatureRelocation=1; every group is resolved by unique .text scan instead of reference addresses.");
    }
    g_executable_base = base;
    g_locator = mgs4::resolver::make_locator(base, mode);
    if (!g_locator.text.valid()) {
        log_line("ERROR: executable .text section is unavailable; no changes were applied.");
        return 0;
    }
    if (g_signature_audit) {
        if (wait_for_group("render extent (audit start)", [] {
                mgs4::resolver::RenderExtent extent{};
                return mgs4::resolver::resolve_render_extent(g_locator,
                                                             &extent);
            }))
            audit_core_signatures();
    }
    if (g_enable_resolution_override &&
        wait_for_group("resolution getters, setter and render-state mirrors",
                       [] {
                           return mgs4::resolver::resolve_resolution(
                               g_locator, &g_resolution);
                       })) {
        g_render_extent = g_resolution.render_extent;
        force_resolution_getters(render_width, render_height);
        install_resolution_hook();
    }
    if (enable_ultrawide) {
        bool native_hook_started = false;
        if (g_native_camera_fov_requested)
            native_hook_started = install_native_camera_fov_hook();
        if (g_experimental_cinematic_fov_requested) {
            if (native_hook_started) {
                install_cinematic_camera_owner_hook();
            } else {
                log_line("WARNING: experimental cinematic FOV requires the native camera hook and remains disabled for this run.");
            }
        }
        install_engine_hook();
        if (g_native_camera_fov_requested) {
            log_line("Experimental native FOV requested. The primary camera route owns the multiplier; common-setter FOV remains the automatic fallback only if the native hook cannot start.");
        } else {
            log_line("Experimental native FOV disabled by the user. Ultrawide aspect correction remains active with the game's original vertical FOV.");
        }
    }
    if (controller_profile_fix)
        install_controller_profile_fix();
    if (!g_render_extent) {
        mgs4::resolver::RenderExtent extent{};
        if (wait_for_group("render extent", [&] {
                return mgs4::resolver::resolve_render_extent(g_locator,
                                                             &extent);
            }))
            g_render_extent = extent.width;
    }
    if (g_render_extent &&
        wait_for_group("reticle truncation sites", [] {
            return mgs4::resolver::resolve_reticle(g_locator, g_render_extent,
                                                   &g_reticle_sites);
        }))
        fix_reticle_truncation();
    apply_resolution_state();
    // The display-mode hook handles subsequent changes; resolution is not polled.
    Sleep(2000);
    if (enable_supersampling) log_presentation_window_size();
    char projection_message[256] = {};
    std::snprintf(projection_message, sizeof(projection_message),
                  "Projection activity after startup: native camera input scales=%ld; common-setter corrections=%ld (%ld exceeded legacy limits); native camera mode=%s.",
                  InterlockedCompareExchange(&g_native_camera_fov_patches, 0, 0),
                  InterlockedCompareExchange(&g_projection_patches, 0, 0),
                  InterlockedCompareExchange(&g_extended_projection_patches, 0, 0),
                  g_native_camera_fov_active ? "active" :
                      (g_native_camera_fov_requested ? "fallback" : "disabled"));
    log_line(projection_message);
    char signature_message[256] = {};
    std::snprintf(signature_message, sizeof(signature_message),
                  "Signature groups: %ld resolved (%s); %ld unresolved.",
                  InterlockedCompareExchange(&g_resolved_groups, 0, 0),
                  g_locator.mode == mgs4::resolver::Mode::reference
                      ? "reference profile addresses"
                      : "relocated by unique .text match",
                  InterlockedCompareExchange(&g_unresolved_groups, 0, 0));
    log_line(signature_message);
    if (g_locator.mode == mgs4::resolver::Mode::relocate) {
        const auto rva = [base](std::uintptr_t address) {
            return static_cast<unsigned long long>(address ? address - base : 0);
        };
        char resolved_message[512] = {};
        std::snprintf(resolved_message, sizeof(resolved_message),
                      "Resolved RVAs: render extent 0x%llx; resolution setter 0x%llx; getters 0x%llx/0x%llx; camera builder 0x%llx (routes 0x%llx, 0x%llx, 0x%llx); reticle 0x%llx, 0x%llx, 0x%llx, 0x%llx; controller mask 0x%llx.",
                      rva(g_render_extent), rva(g_resolution.setter),
                      rva(g_resolution.width_getter),
                      rva(g_resolution.height_getter), rva(g_camera.builder),
                      rva(g_camera.primary_return),
                      rva(g_camera.cinematic_source_return),
                      rva(g_camera.cinematic_final_return),
                      rva(g_reticle_sites[0]), rva(g_reticle_sites[1]),
                      rva(g_reticle_sites[2]), rva(g_reticle_sites[3]),
                      rva(g_controller_mask));
        log_line(resolved_message);
    }
    log_line("Initial state applied; patch thread finished without a polling loop.");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void*) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, patch_thread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
