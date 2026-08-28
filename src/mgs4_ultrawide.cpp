#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "MinHook.h"

#if defined(MGS4ULTRA120_WINMM_PROXY)
extern "C" FARPROC winmm_proxy_resolve_by_name(const char* name);
#endif
static float g_target_aspect = 43.0f / 18.0f;
static volatile LONG g_projection_patches;
static std::uintptr_t g_executable_base;
static std::uint32_t g_target_width = 3440;
static std::uint32_t g_target_height = 1440;
static float g_fov_multiplier = 1.0f;
static bool g_enable_ultrawide = true;
static bool g_controller_profile_fix = true;
static volatile LONG g_locked_controller_profile;
static volatile LONG g_minhook_state;

using TimeBeginPeriodFn = MMRESULT (WINAPI*)(UINT);
using TimeGetTimeFn = DWORD (WINAPI*)();
using SetProjectionFn = void (__fastcall*)(const float* matrix);
using SetResolutionFn = void (__fastcall*)(std::uint16_t mode, std::int32_t index,
                                           std::uint8_t use_safe_area,
                                           std::uint32_t width,
                                           std::uint32_t height);
static SetProjectionFn g_original_set_projection;
static SetResolutionFn g_original_set_resolution;
using SetDetectedProfileFn = void (__fastcall*)(std::int32_t);
static SetDetectedProfileFn g_original_set_detected_profile;

static void apply_resolution_state();
static bool initialize_minhook();
static void log_line(const char* message);

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

// The PC port can spuriously select profile 0 (keyboard) while an XInput slot
// remains connected. Its subsequent keyboard merge then neutralizes otherwise
// valid controller axes. Preserve whichever native controller family the game
// detected (profiles 1..7) until every controller slot is disconnected. This
// neither synthesizes input nor periodically rewrites game state.
static void __fastcall hooked_set_detected_profile(std::int32_t profile) {
    const auto base = g_executable_base;
    if (g_controller_profile_fix && base) {
        const LONG connected_mask =
            *reinterpret_cast<volatile LONG*>(base + 0x23d2dbc0);
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

// Historical D3D12 UI experiment retained temporarily for source archaeology.
// It is compiled out of release builds: identifying one shared shader was not
// enough to distinguish HUD from full-screen effects safely.
#if 0
// The game's UI vertex shader is shared by the D3D11 and D3D12 renderers. Its
// DXBC container is stable for the supported executable. Matching the exact
// shader provides an experimental 16:9 safe-area path without changing the
// compositor's global canvas. Some full-screen effects share this shader, so
// the path is opt-in until those draws can be distinguished from menu/HUD draws.
static constexpr std::size_t ui_shader_size = 948;
static constexpr unsigned char ui_shader_dxbc_header[20] = {
    0x44, 0x58, 0x42, 0x43, 0xb2, 0xfd, 0xf6, 0xc0, 0x4d, 0x44,
    0xbd, 0x73, 0xc9, 0x70, 0x91, 0x06, 0xa1, 0xc8, 0x5f, 0xa8,
};

static bool is_ui_shader(const D3D12_SHADER_BYTECODE& shader) {
    return shader.pShaderBytecode && shader.BytecodeLength == ui_shader_size &&
        std::memcmp(shader.pShaderBytecode, ui_shader_dxbc_header,
                    sizeof(ui_shader_dxbc_header)) == 0;
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
using SetPipelineStateFn = void (STDMETHODCALLTYPE*)(
    ID3D12GraphicsCommandList*, ID3D12PipelineState*);

static D3D12CreateDeviceFn g_original_d3d12_create_device;
static CreateGraphicsPipelineStateFn g_original_create_graphics_pso;
static CreateCommandListFn g_original_create_command_list;
static CommandListResetFn g_original_command_list_reset;
static CommandListClearStateFn g_original_command_list_clear_state;
static DrawInstancedFn g_original_draw_instanced;
static DrawIndexedInstancedFn g_original_draw_indexed_instanced;
static RSSetViewportsFn g_original_rs_set_viewports;
static SetPipelineStateFn g_original_set_pipeline_state;
static volatile LONG g_d3d12_device_hooks_installed;
static volatile LONG g_d3d12_command_hooks_installed;
static volatile LONG g_d3d12_export_hook_state;
static volatile LONG g_ui_pipeline_count;
static volatile LONG g_ui_draw_count;

struct CommandListState {
    void* volatile command_list;
    void* volatile pipeline_state;
    D3D12_VIEWPORT viewport;
    volatile LONG has_viewport;
};

static constexpr std::size_t state_table_size = 256;
static constexpr std::size_t pipeline_table_size = 128;
static CommandListState g_command_states[state_table_size] = {};
static void* volatile g_ui_pipelines[pipeline_table_size] = {};

static std::size_t pointer_hash(const void* pointer, std::size_t table_size) {
    const auto value = reinterpret_cast<std::uintptr_t>(pointer);
    return static_cast<std::size_t>(((value >> 4) ^ (value >> 17)) % table_size);
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

static void remember_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        void* volatile* entry = &g_ui_pipelines[(slot + probe) % pipeline_table_size];
        void* existing = InterlockedCompareExchangePointer(entry, nullptr, nullptr);
        if (existing == pipeline) return;
        if (!existing && InterlockedCompareExchangePointer(entry, pipeline, nullptr) == nullptr) {
            const LONG count = InterlockedIncrement(&g_ui_pipeline_count);
            if (count == 1)
                log_line(g_constrain_ui
                    ? "D3D12: shared UI/effect shader recognized; experimental 16:9 safe area active."
                    : "D3D12: shared UI/effect shader recognized; safe area disabled.");
            return;
        }
    }
    log_line("D3D12 ERROR: UI pipeline table is full.");
}

static bool is_ui_pipeline(ID3D12PipelineState* pipeline) {
    if (!pipeline) return false;
    std::size_t slot = pointer_hash(pipeline, pipeline_table_size);
    for (std::size_t probe = 0; probe < pipeline_table_size; ++probe) {
        void* existing = InterlockedCompareExchangePointer(
            &g_ui_pipelines[(slot + probe) % pipeline_table_size], nullptr, nullptr);
        if (existing == pipeline) return true;
        if (!existing) return false;
    }
    return false;
}

static bool make_ui_viewport(CommandListState* state, D3D12_VIEWPORT* safe) {
    if (!g_constrain_ui || !state || !safe ||
        InterlockedCompareExchange(&state->has_viewport, 0, 0) == 0)
        return false;
    auto* pipeline = reinterpret_cast<ID3D12PipelineState*>(
        InterlockedCompareExchangePointer(&state->pipeline_state, nullptr, nullptr));
    if (!is_ui_pipeline(pipeline)) return false;

    const D3D12_VIEWPORT original = state->viewport;
    const float safe_width = original.Height * (16.0f / 9.0f);
    if (!std::isfinite(safe_width) || original.Height <= 0.0f ||
        original.Width <= safe_width + 0.5f)
        return false;
    *safe = original;
    safe->TopLeftX += (original.Width - safe_width) * 0.5f;
    safe->Width = safe_width;
    return true;
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

static void STDMETHODCALLTYPE hooked_set_pipeline_state(
    ID3D12GraphicsCommandList* command_list, ID3D12PipelineState* pipeline) {
    if (CommandListState* state = command_state(command_list, true))
        InterlockedExchangePointer(&state->pipeline_state, pipeline);
    g_original_set_pipeline_state(command_list, pipeline);
}

static void STDMETHODCALLTYPE hooked_draw_instanced(
    ID3D12GraphicsCommandList* command_list, UINT vertex_count, UINT instance_count,
    UINT first_vertex, UINT first_instance) {
    CommandListState* state = command_state(command_list, false);
    D3D12_VIEWPORT safe = {};
    if (make_ui_viewport(state, &safe)) {
        const D3D12_VIEWPORT original = state->viewport;
        g_original_rs_set_viewports(command_list, 1, &safe);
        g_original_draw_instanced(command_list, vertex_count, instance_count,
                                  first_vertex, first_instance);
        g_original_rs_set_viewports(command_list, 1, &original);
        InterlockedIncrement(&g_ui_draw_count);
        return;
    }
    g_original_draw_instanced(command_list, vertex_count, instance_count,
                              first_vertex, first_instance);
}

static void STDMETHODCALLTYPE hooked_draw_indexed_instanced(
    ID3D12GraphicsCommandList* command_list, UINT index_count, UINT instance_count,
    UINT first_index, INT base_vertex, UINT first_instance) {
    CommandListState* state = command_state(command_list, false);
    D3D12_VIEWPORT safe = {};
    if (make_ui_viewport(state, &safe)) {
        const D3D12_VIEWPORT original = state->viewport;
        g_original_rs_set_viewports(command_list, 1, &safe);
        g_original_draw_indexed_instanced(command_list, index_count, instance_count,
                                          first_index, base_vertex, first_instance);
        g_original_rs_set_viewports(command_list, 1, &original);
        InterlockedIncrement(&g_ui_draw_count);
        return;
    }
    g_original_draw_indexed_instanced(command_list, index_count, instance_count,
                                      first_index, base_vertex, first_instance);
}
#endif

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

#if 0
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
    const bool pipeline_ok = create_and_enable_hook(
        vtable[25], reinterpret_cast<void*>(&hooked_set_pipeline_state),
        reinterpret_cast<void**>(&g_original_set_pipeline_state), "D3D12 SetPipelineState");
    bool state_ok = create_and_enable_hook(
        vtable[10], reinterpret_cast<void*>(&hooked_command_list_reset),
        reinterpret_cast<void**>(&g_original_command_list_reset), "D3D12 Reset");
    state_ok &= create_and_enable_hook(
        vtable[11], reinterpret_cast<void*>(&hooked_command_list_clear_state),
        reinterpret_cast<void**>(&g_original_command_list_clear_state), "D3D12 ClearState");
    bool draws_ok = false;
    if (viewport_ok && pipeline_ok) {
        draws_ok = create_and_enable_hook(
            vtable[12], reinterpret_cast<void*>(&hooked_draw_instanced),
            reinterpret_cast<void**>(&g_original_draw_instanced), "D3D12 DrawInstanced");
        draws_ok &= create_and_enable_hook(
            vtable[13], reinterpret_cast<void*>(&hooked_draw_indexed_instanced),
            reinterpret_cast<void**>(&g_original_draw_indexed_instanced),
            "D3D12 DrawIndexedInstanced");
    }
    const bool okay = viewport_ok && pipeline_ok && state_ok && draws_ok;
    if (okay)
        log_line("D3D12: command-list hooks installed.");
    else
        InterlockedExchange(&g_d3d12_command_hooks_installed, -1);
}

static HRESULT STDMETHODCALLTYPE hooked_create_graphics_pso(
    ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
    REFIID riid, void** pipeline) {
    const HRESULT result = g_original_create_graphics_pso(device, desc, riid, pipeline);
    if (SUCCEEDED(result) && pipeline && *pipeline && desc && is_ui_shader(desc->VS))
        remember_ui_pipeline(reinterpret_cast<ID3D12PipelineState*>(*pipeline));
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
        if (CommandListState* state = command_state(graphics, true))
            InterlockedExchangePointer(&state->pipeline_state, initial_state);
        install_command_list_hooks(graphics);
    }
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
    const bool okay = pso_ok && lists_ok;
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

static bool ui_hook_requested_from_config() {
    const LONG cached = InterlockedCompareExchange(&g_ui_hook_requested_state, 0, 0);
    if (cached != 0) return cached > 0;
    char ini_path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, ini_path, MAX_PATH);
    if (char* slash = std::strrchr(ini_path, '\\'))
        std::strcpy(slash + 1, "mgs4_ultrawide.ini");
    const bool requested =
        GetPrivateProfileIntA("Patch", "UltrawideEnabled", 1, ini_path) != 0 &&
        GetPrivateProfileIntA("Ultrawide", "ConstrainUITo16x9", 0, ini_path) != 0;
    InterlockedCompareExchange(&g_ui_hook_requested_state, requested ? 1 : -1, 0);
    return requested;
}
#endif

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

static bool near_zero(float x) {
    return std::isfinite(x) && std::fabs(x) < 0.00001f;
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
    const bool finite_scale = std::isfinite(x) && std::isfinite(y) &&
                              std::fabs(x) >= 0.25f && std::fabs(x) <= 8.0f &&
                              std::fabs(y) >= 0.25f && std::fabs(y) <= 12.0f;
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
    if (finite_scale && perspective_shape && known_aspect) {
        copy[5] = std::copysign(std::fabs(y) / g_fov_multiplier, y);
        copy[0] = std::copysign(std::fabs(copy[5]) / g_target_aspect, x);
        InterlockedIncrement(&g_projection_patches);
    }
    g_original_set_projection(copy);
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
           nt->OptionalHeader.SizeOfImage == 0x241be000;
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
        VirtualProtect(target, sizeof(replacement),
                       final_code_protection(old_protection), &ignored);
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

static bool install_controller_profile_fix(std::uintptr_t base) {
    constexpr std::uintptr_t setter_rva = 0x750ec0;
    constexpr unsigned char expected[] =
        {0x40, 0x53, 0x48, 0x83, 0xec, 0x20, 0x8b, 0xd9};
    auto* target = reinterpret_cast<unsigned char*>(base + setter_rva);
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        if (std::memcmp(target, expected, sizeof(expected)) == 0) break;
        if (attempt == 199) {
            log_line("ERROR: controller-profile target did not decrypt in time.");
            return false;
        }
        Sleep(25);
    }

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

    if (g_enable_ultrawide) {
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
    }
}

static DWORD WINAPI patch_thread(void*) {
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
    const bool allow_unsupported = GetPrivateProfileIntA(
        "Patch", "AllowUnsupportedExecutable", 0, ini_path) != 0;
    const bool controller_profile_fix = GetPrivateProfileIntA(
        "Input", "ControllerProfileFixEnabled", 1, ini_path) != 0;
    char fov_text[32] = {};
    GetPrivateProfileStringA("Ultrawide", "FOVMultiplier", "1.000", fov_text,
                             sizeof(fov_text), ini_path);
    float fov_multiplier = 1.0f;
    const bool fov_valid = parse_ini_decimal(fov_text, &fov_multiplier);
    if (enable_ultrawide &&
        (!width || !height || !fov_valid ||
         fov_multiplier < 0.5f || fov_multiplier > 2.0f)) {
        char invalid_message[512] = {};
        std::snprintf(invalid_message, sizeof(invalid_message),
                      "ERROR: invalid ultrawide configuration in %s: Width=%u, Height=%u, FOVMultiplier='%s' (accepted FOV range 0.500-2.000).",
                      ini_path, width, height, fov_text);
        log_line(invalid_message);
        return 0;
    }
    g_enable_ultrawide = enable_ultrawide;
    g_controller_profile_fix = controller_profile_fix;
    g_target_width = width;
    g_target_height = height;
    g_fov_multiplier = fov_multiplier;
    g_target_aspect = static_cast<float>(width) / static_cast<float>(height);

    char settings_message[320] = {};
    std::snprintf(settings_message, sizeof(settings_message),
                  "Configuration: ultrawide %s (%ux%u, aspect %.6f, FOVMultiplier %.3f), controller-profile fix %s. FPS timing is delegated to MGSFPSUnlock.",
                  enable_ultrawide ? "on" : "off", width, height,
                  g_target_aspect, g_fov_multiplier,
                  controller_profile_fix ? "on" : "off");
    log_line(settings_message);

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!supported_executable(base)) {
        if (!allow_unsupported) {
            log_line("ERROR: unrecognized mgs4.exe version; no offsets were applied. Set AllowUnsupportedExecutable=1 only if you accept crash/corruption risk.");
            return 0;
        }
        log_line("WARNING: unsupported executable override enabled. Known RVAs will be attempted under user responsibility; hook signatures are still checked.");
    }
    g_executable_base = base;
    if (enable_ultrawide) {
        force_resolution_getters(base, width, height);
        install_resolution_hook(base);
        install_engine_hook(base);
    }
    if (controller_profile_fix)
        install_controller_profile_fix(base);
    apply_resolution_state();
    // The display-mode hook handles subsequent changes; resolution is not polled.
    Sleep(2000);
    char projection_message[128] = {};
    std::snprintf(projection_message, sizeof(projection_message),
                  "Projection matrices adjusted after startup: %ld.",
                  InterlockedCompareExchange(&g_projection_patches, 0, 0));
    log_line(projection_message);
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
