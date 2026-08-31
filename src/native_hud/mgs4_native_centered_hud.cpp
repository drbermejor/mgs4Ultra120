#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#define MGS4_RETURN_ADDRESS() _ReturnAddress()
#else
#define MGS4_RETURN_ADDRESS() __builtin_return_address(0)
#endif

#include <MinHook.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <limits>

#include "native_hud_math.h"

namespace {

using mgs4::native_hud::Canvas;

constexpr DWORD kSupportedTimeDateStamp = 0x6a8cfc47;
constexpr DWORD kSupportedSizeOfImage = 0x241be000;
constexpr std::uintptr_t kRenderWidthRva = 0x1b00000;
constexpr std::uintptr_t kRenderHeightRva = 0x1b00004;
constexpr std::uintptr_t kHudLayoutRva = 0x439810;
constexpr std::uintptr_t kPhysicalRectEmitterRva = 0x0be090;
constexpr std::uintptr_t kSemanticOwnerRectRva = 0x4da5b0;
constexpr std::uintptr_t kNativeSolidNodeRva = 0x425520;
constexpr std::uintptr_t kNativeNodeDispatcherRva = 0x4278b0;
constexpr std::uintptr_t kNativeLayerTraversalRva = 0x428510;

constexpr std::uint32_t kSubtitlePhysicalRectCaller = 0x00084b5c;
constexpr std::uint32_t kMoviePhysicalRectCaller = 0x00096c80;
constexpr std::uint32_t kTvMoviePhysicalRectCaller = 0x00e367bb;
constexpr std::uint32_t kCamouflagePreviewCaller = 0x004f8b3b;
constexpr std::uint32_t kItemPreviewCaller = 0x004fc9e3;
constexpr std::uint32_t kDrebinShopPreviewCaller = 0x00504363;
constexpr std::uint32_t kWeaponPreviewCaller = 0x005085a3;
constexpr std::uint32_t kNormalLayerTraversalCaller = 0x00428643;

constexpr std::uint32_t kLoadSaveConfirmationResource = 0x00298bf7;
constexpr std::uint32_t kLoadSaveConfirmationAllocationBytes = 0x000029a8;
constexpr std::uint32_t kTitleMenuResource = 0x003d7604;
constexpr std::uint32_t kTitleMenuAllocationBytes = 0x000004d0;

struct VerifiedFullscreenSolid {
    std::uint32_t resource{};
    std::uint32_t allocation_bytes{};
    std::uint32_t node_type_flags{};
    std::uint32_t node_key{};
    std::uint32_t raw_color{};
    std::uint32_t raw_item_index{};
    std::uint32_t identity_seed_key{};
    std::uint32_t identity_seed_raw_item{};
};

// Output-covering children verified in live native traversals. Geometry alone
// never opts a node into the correction: every semantic field below and all
// traversal guards in hooked_native_solid_node must agree.
constexpr std::array<VerifiedFullscreenSolid, 3>
    kVerifiedFullscreenSolids{{
        {kLoadSaveConfirmationResource,
         kLoadSaveConfirmationAllocationBytes, 0x43420002,
         0x00000887, 0x0808086e, 8, 0x00e993cd, 7},
        {kTitleMenuResource, kTitleMenuAllocationBytes,
         0x43420002, 0x006b7b73, 0x08080880, 3, 0x0011565b, 2},
        {kTitleMenuResource, kTitleMenuAllocationBytes,
         0x43420002, 0x006b7b88, 0xffffff80, 5, 0x00115670, 4},
    }};

constexpr unsigned char kHudLayoutPrologue[] = {
    0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x6c,
};
constexpr unsigned char kPhysicalRectEmitterBytes[] = {
    0x0f, 0xb7, 0x44, 0x24, 0x28, 0x66, 0x89, 0x41,
    0x0e, 0x48, 0x8d, 0x41, 0x10, 0xc6, 0x01, 0x0a,
    0x66, 0x89, 0x51, 0x08, 0x66, 0x44, 0x89, 0x41,
    0x0a, 0x66, 0x44, 0x89, 0x49, 0x0c, 0xc3,
};
constexpr unsigned char kSemanticOwnerRectBytes[] = {
    0x40, 0x53, 0x48, 0x83, 0xec, 0x30, 0x4c, 0x8b, 0xd9,
};
constexpr unsigned char kNativeSolidNodeBytes[] = {
    0x48, 0x83, 0xec, 0x38, 0x48, 0x8b, 0x4a, 0x1c,
    0x4c, 0x8b, 0xca, 0x44, 0x0f, 0xb6, 0x52, 0x18,
    0x4d, 0x8b, 0xd8, 0x48, 0x8b, 0x42, 0x24, 0x45,
    0x03, 0xd2, 0xba, 0xff, 0x00, 0x00, 0x00,
};
constexpr unsigned char kNativeNodeDispatcherBytes[] = {
    0x48, 0x89, 0x6c, 0x24, 0x20, 0x56, 0x57, 0x41, 0x57,
    0x48, 0x83, 0xec, 0x50, 0x41, 0x8b, 0x00, 0x49, 0x8b,
    0xe9, 0x48, 0x89, 0x5c, 0x24, 0x78, 0x49, 0x8b, 0xf8,
};
constexpr unsigned char kNativeLayerTraversalBytes[] = {
    0x48, 0x89, 0x5c, 0x24, 0x18, 0x48, 0x89, 0x6c, 0x24,
    0x20, 0x56, 0x57, 0x41, 0x57, 0x48, 0x83, 0xec, 0x20,
    0x0f, 0xb7, 0x7a, 0x10, 0x45, 0x33, 0xff, 0x48, 0x8b, 0xf2,
};

using HudLayoutFn = std::uint8_t(__fastcall*)(std::uintptr_t, std::int32_t,
                                              std::int32_t, std::int32_t,
                                              std::int32_t);
using PhysicalRectEmitterFn = std::uint8_t*(__fastcall*)(
    std::uint8_t*, std::uint16_t, std::uint16_t, std::uint16_t,
    std::uint16_t);
using SemanticOwnerRectFn = std::uint64_t(__fastcall*)(
    std::uint64_t, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
using NativeSolidNodeFn = std::uintptr_t(__fastcall*)(
    std::uintptr_t, std::uintptr_t, std::uintptr_t);
using NativeNodeDispatcherFn = std::uintptr_t(__fastcall*)(
    std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
using NativeLayerTraversalFn = void(__fastcall*)(std::uintptr_t,
                                                  std::uintptr_t);

struct Configuration {
    bool center_subtitles{true};
    bool center_movies{true};
    bool center_tv_movies{true};
    bool center_inventory_previews{true};
    bool expand_verified_fullscreen_backgrounds{true};
};

struct NodeTraversalContext {
    bool active{};
    bool transform_known{};
    bool transform_identity{};
    std::uintptr_t parent{};
    std::uintptr_t layer{};
    std::uint32_t dispatch_ordinal{};
    std::uint32_t last_raw_item_index{UINT32_MAX};
    std::uint32_t identity_seed_key{};
    std::uint32_t identity_seed_raw_item{UINT32_MAX};
    std::uint16_t item_count{};
    std::uint16_t priority{};
    std::uint16_t flags{};
};

struct DispatcherContext {
    bool active{};
    bool normal{};
    std::uintptr_t parent{};
    std::uintptr_t node{};
    std::uint32_t raw_item_index{UINT32_MAX};
};

HudLayoutFn g_original_layout;
PhysicalRectEmitterFn g_original_physical_rect;
SemanticOwnerRectFn g_original_semantic_owner_rect;
NativeSolidNodeFn g_original_native_solid_node;
NativeNodeDispatcherFn g_original_native_node_dispatcher;
NativeLayerTraversalFn g_original_native_layer_traversal;
std::uintptr_t g_base;
wchar_t g_game_dir[MAX_PATH]{};
wchar_t g_ini_path[MAX_PATH]{};
wchar_t g_log_path[MAX_PATH]{};
Configuration g_config{};
volatile LONG g_log_lock;
thread_local NodeTraversalContext g_node_traversal_context{};
thread_local DispatcherContext g_dispatcher_context{};

void log_line(const char* format, ...) {
    while (InterlockedCompareExchange(&g_log_lock, 1, 0) != 0) Sleep(0);
    FILE* file = nullptr;
    _wfopen_s(&file, g_log_path, L"a");
    if (file) {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        std::fprintf(file, "[%02u:%02u:%02u.%03u] ", now.wHour,
                     now.wMinute, now.wSecond, now.wMilliseconds);
        va_list arguments;
        va_start(arguments, format);
        std::vfprintf(file, format, arguments);
        va_end(arguments);
        std::fputc('\n', file);
        std::fclose(file);
    }
    InterlockedExchange(&g_log_lock, 0);
}

bool resolve_paths() {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    wchar_t* slash = std::wcsrchr(executable, L'\\');
    if (!slash) return false;
    *slash = L'\0';
    wcsncpy_s(g_game_dir, executable, _TRUNCATE);
    std::swprintf(g_ini_path, MAX_PATH,
                  L"%ls\\mgs4_native_centered_hud.ini",
                  executable);
    std::swprintf(g_log_path, MAX_PATH,
                  L"%ls\\mgs4_native_centered_hud.log",
                  executable);
    return true;
}

bool duplicate_native_hud_install_present() {
    wchar_t root_path[MAX_PATH]{};
    wchar_t scripts_path[MAX_PATH]{};
    std::swprintf(root_path, MAX_PATH,
                  L"%ls\\MGS4NativeCenteredHUD.asi", g_game_dir);
    std::swprintf(scripts_path, MAX_PATH,
                  L"%ls\\scripts\\MGS4NativeCenteredHUD.asi", g_game_dir);
    return GetFileAttributesW(root_path) != INVALID_FILE_ATTRIBUTES &&
           GetFileAttributesW(scripts_path) != INVALID_FILE_ATTRIBUTES;
}

bool supported_executable() {
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        g_base + static_cast<std::uintptr_t>(dos->e_lfanew));
    return nt->Signature == IMAGE_NT_SIGNATURE &&
           nt->FileHeader.TimeDateStamp == kSupportedTimeDateStamp &&
           nt->OptionalHeader.SizeOfImage == kSupportedSizeOfImage;
}

bool bytes_match(std::uintptr_t rva, const unsigned char* expected,
                 std::size_t expected_size) {
    return std::memcmp(reinterpret_cast<const void*>(g_base + rva), expected,
                       expected_size) == 0;
}

bool wait_for_core_native_code() {
    for (unsigned attempt = 0; attempt < 400; ++attempt) {
        if (bytes_match(kHudLayoutRva, kHudLayoutPrologue,
                        sizeof(kHudLayoutPrologue)) &&
            bytes_match(kPhysicalRectEmitterRva,
                        kPhysicalRectEmitterBytes,
                        sizeof(kPhysicalRectEmitterBytes))) {
            return true;
        }
        Sleep(25);
    }
    return false;
}

bool config_flag(const wchar_t* name, bool default_value) {
    return GetPrivateProfileIntW(L"NativeHUD", name,
                                 default_value ? 1 : 0, g_ini_path) != 0;
}

void load_configuration() {
    g_config.center_subtitles = config_flag(L"CenterSubtitles", true);
    g_config.center_movies = config_flag(L"CenterMovies", true);
    g_config.center_tv_movies = config_flag(L"CenterTVMovies", true);
    g_config.center_inventory_previews =
        config_flag(L"CenterInventoryPreviews", true);
    g_config.expand_verified_fullscreen_backgrounds =
        config_flag(L"ExpandVerifiedFullscreenBackgrounds", true);
}

Canvas current_canvas() {
    const auto* width_ptr = reinterpret_cast<volatile const std::int32_t*>(
        g_base + kRenderWidthRva);
    const auto* height_ptr = reinterpret_cast<volatile const std::int32_t*>(
        g_base + kRenderHeightRva);
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        const std::int32_t width_before = *width_ptr;
        const std::int32_t height_before = *height_ptr;
        const std::int32_t width_after = *width_ptr;
        const std::int32_t height_after = *height_ptr;
        if (width_before == width_after && height_before == height_after) {
            return mgs4::native_hud::make_canvas(width_before,
                                                 height_before);
        }
        YieldProcessor();
    }
    // Never combine dimensions sampled from two resolution epochs.
    return {};
}

std::uint32_t to_caller_rva(std::uintptr_t address) {
    if (address < g_base || address - g_base > 0xffffffffull)
        return 0xffffffffu;
    return static_cast<std::uint32_t>(address - g_base);
}

template <typename T>
T read_unaligned(std::uintptr_t address) {
    T value{};
    std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
    return value;
}

template <typename T>
void write_unaligned(std::uintptr_t address, const T& value) {
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(value));
}

std::uint8_t set_field(std::uintptr_t object, std::uintptr_t offset,
                       std::int32_t value) {
    auto* field = reinterpret_cast<std::int32_t*>(object + offset);
    if (*field == value) return 0;
    *field = value;
    return 1;
}

std::uint8_t write_centered_layout(std::uintptr_t object, std::int32_t x,
                                   std::int32_t y, std::int32_t width,
                                   std::int32_t height,
                                   const Canvas& canvas) {
    std::uint8_t changed = 0;
    changed += set_field(object, 0x200,
                         mgs4::native_hud::logical_x(x, canvas));
    changed += set_field(object, 0x204,
                         mgs4::native_hud::logical_y(y, canvas));
    changed += set_field(object, 0x208,
                         mgs4::native_hud::logical_width(width, canvas));
    changed += set_field(object, 0x20c,
                         mgs4::native_hud::logical_y(height, canvas));
    changed += set_field(object, 0x210, x);
    changed += set_field(object, 0x214, y);
    changed += set_field(object, 0x218, x + width);
    changed += set_field(object, 0x21c, y + height);
    if (changed)
        *reinterpret_cast<std::uint32_t*>(object + 0x18) |= 0x220000;
    return changed;
}

std::uint8_t write_logical_root_layout(std::uintptr_t object,
                                       const Canvas& canvas) {
    const auto logical =
        mgs4::native_hud::expanded_logical_canvas(canvas);
    std::uint8_t changed = 0;
    changed += set_field(object, 0x200, 0);
    changed += set_field(object, 0x204, 0);
    changed += set_field(object, 0x208, canvas.output_width);
    changed += set_field(object, 0x20c, canvas.output_height);
    changed += set_field(object, 0x210, logical.left);
    changed += set_field(object, 0x214, logical.top);
    changed += set_field(object, 0x218, logical.right);
    changed += set_field(object, 0x21c, logical.bottom);
    if (changed)
        *reinterpret_cast<std::uint32_t*>(object + 0x18) |= 0x220000;
    return changed;
}

bool is_full_logical_canvas(std::int32_t x, std::int32_t y,
                            std::int32_t width, std::int32_t height) {
    return x == 0 && y == 0 && width == 1280 && height == 720;
}

std::uint8_t __fastcall hooked_layout(std::uintptr_t object, std::int32_t x,
                                      std::int32_t y, std::int32_t width,
                                      std::int32_t height) {
    if (!object) return g_original_layout(object, x, y, width, height);
    const Canvas canvas = current_canvas();
    if (!canvas.active())
        return g_original_layout(object, x, y, width, height);
    if (!mgs4::native_hud::layout_arithmetic_is_safe(x, y, width, height,
                                                      canvas))
        return g_original_layout(object, x, y, width, height);
    if (is_full_logical_canvas(x, y, width, height))
        return write_logical_root_layout(object, canvas);
    return write_centered_layout(object, x, y, width, height, canvas);
}

bool direct_surface_enabled(std::uint32_t caller) {
    if (caller == kSubtitlePhysicalRectCaller)
        return g_config.center_subtitles;
    if (caller == kMoviePhysicalRectCaller)
        return g_config.center_movies;
    if (caller == kTvMoviePhysicalRectCaller)
        return g_config.center_tv_movies;
    return false;
}

bool fits_u16(std::int32_t value) {
    return value >= 0 && value <= 0xffff;
}

std::uint8_t* __fastcall hooked_physical_rect(
    std::uint8_t* destination, std::uint16_t x, std::uint16_t y,
    std::uint16_t width, std::uint16_t height) {
    const std::uint32_t caller =
        to_caller_rva(
            reinterpret_cast<std::uintptr_t>(MGS4_RETURN_ADDRESS()));
    const Canvas canvas = current_canvas();
    if (!canvas.active() || !direct_surface_enabled(caller)) {
        return g_original_physical_rect(destination, x, y, width, height);
    }
    const std::int32_t mapped_x =
        mgs4::native_hud::physical_x(x, canvas);
    const std::int32_t mapped_width =
        mgs4::native_hud::physical_width(width, canvas);
    if (!fits_u16(mapped_x) || !fits_u16(mapped_width) ||
        (width != 0 && mapped_width <= 0)) {
        return g_original_physical_rect(destination, x, y, width, height);
    }
    return g_original_physical_rect(
        destination, static_cast<std::uint16_t>(mapped_x), y,
        static_cast<std::uint16_t>(mapped_width), height);
}

bool is_inventory_preview_caller(std::uint32_t caller) {
    return caller == kCamouflagePreviewCaller ||
           caller == kItemPreviewCaller ||
           caller == kDrebinShopPreviewCaller ||
           caller == kWeaponPreviewCaller;
}

std::uint64_t __fastcall hooked_semantic_owner_rect(
    std::uint64_t handle, std::int32_t left, std::int32_t top,
    std::int32_t right, std::int32_t bottom) {
    const std::uint32_t caller =
        to_caller_rva(
            reinterpret_cast<std::uintptr_t>(MGS4_RETURN_ADDRESS()));
    const Canvas canvas = current_canvas();
    if (!g_config.center_inventory_previews || !canvas.active() ||
        !is_inventory_preview_caller(caller)) {
        return g_original_semantic_owner_rect(handle, left, top, right,
                                              bottom);
    }
    const auto mapped = mgs4::native_hud::uniform_safe_rect(
        {left, top, right, bottom}, canvas);
    return g_original_semantic_owner_rect(
        handle, mapped.left, mapped.top, mapped.right, mapped.bottom);
}

bool is_node_probe_target(std::uint32_t resource, std::uint32_t bytes) {
    return (resource == kLoadSaveConfirmationResource &&
            bytes == kLoadSaveConfirmationAllocationBytes) ||
           (resource == kTitleMenuResource &&
            bytes == kTitleMenuAllocationBytes);
}

bool parent_uses_expanded_logical_root(std::uintptr_t parent,
                                       const Canvas& canvas) {
    if (!parent || !canvas.active()) return false;
    const auto logical = mgs4::native_hud::expanded_logical_canvas(canvas);
    const auto snapshot = [parent] {
        std::array<std::int32_t, 8> values{};
        const auto* fields = reinterpret_cast<volatile const std::int32_t*>(
            parent + 0x200);
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = fields[index];
        return values;
    };
    const auto first = snapshot();
    MemoryBarrier();
    const auto second = snapshot();
    if (first != second) return false;
    return second == std::array<std::int32_t, 8>{
        0, 0, canvas.output_width, canvas.output_height,
        logical.left, logical.top, logical.right, logical.bottom};
}

bool is_verified_fullscreen_solid(std::uint32_t resource,
                                  std::uint32_t bytes,
                                  std::uint32_t node_type_flags,
                                  std::uint32_t node_key,
                                  std::uint32_t raw_color,
                                  std::uint32_t raw_item_index,
                                  std::uint32_t identity_seed_key,
                                  std::uint32_t identity_seed_raw_item) {
    return std::any_of(kVerifiedFullscreenSolids.begin(),
                       kVerifiedFullscreenSolids.end(),
                       [&](const VerifiedFullscreenSolid& solid) {
        return solid.resource == resource &&
            solid.allocation_bytes == bytes &&
            solid.node_type_flags == node_type_flags &&
            solid.node_key == node_key && solid.raw_color == raw_color &&
            solid.raw_item_index == raw_item_index &&
            solid.identity_seed_key == identity_seed_key &&
            solid.identity_seed_raw_item == identity_seed_raw_item;
    });
}

std::uintptr_t __fastcall hooked_native_solid_node(
    std::uintptr_t parent, std::uintptr_t node,
    std::uintptr_t command_stream) {
    if (!g_config.expand_verified_fullscreen_backgrounds || !parent || !node ||
        !command_stream) {
        return g_original_native_solid_node(parent, node, command_stream);
    }

    // A solid reached through cache/prepass machinery or a nested dispatcher
    // must never inherit transformation evidence from an earlier layer item.
    if (!g_node_traversal_context.active ||
        g_node_traversal_context.parent != parent ||
        !g_node_traversal_context.transform_known ||
        !g_node_traversal_context.transform_identity ||
        g_node_traversal_context.priority != 0 ||
        g_node_traversal_context.flags != 0 ||
        !g_dispatcher_context.active || !g_dispatcher_context.normal ||
        g_dispatcher_context.parent != parent ||
        g_dispatcher_context.node != node) {
        return g_original_native_solid_node(parent, node, command_stream);
    }

    // The native handler reads node+0x18..+0x2b synchronously and does not
    // retain the pointer. Snapshot before classifying and modify only this
    // private copy, eliminating shared mutation and a check/copy race.
    alignas(8) std::array<std::uint8_t, 0x2c> copy{};
    std::memcpy(copy.data(), reinterpret_cast<const void*>(node), copy.size());
    const auto copy_address =
        reinterpret_cast<std::uintptr_t>(copy.data());

    const std::uint32_t resource =
        read_unaligned<std::uint32_t>(parent + 0x1c);
    const std::uint32_t bytes =
        read_unaligned<std::uint32_t>(parent + 0x34);
    const std::uint32_t node_type_flags =
        read_unaligned<std::uint32_t>(copy_address);
    const std::uint32_t node_key =
        read_unaligned<std::uint32_t>(copy_address + 0x04);
    const std::uint32_t raw_color =
        read_unaligned<std::uint32_t>(copy_address + 0x18);
    const Canvas canvas = current_canvas();
    if ((node_type_flags & 0x0fu) != 2 ||
        !is_verified_fullscreen_solid(
            resource, bytes, node_type_flags, node_key, raw_color,
            g_dispatcher_context.raw_item_index,
            g_node_traversal_context.identity_seed_key,
            g_node_traversal_context.identity_seed_raw_item) ||
        !parent_uses_expanded_logical_root(parent, canvas) ||
        read_unaligned<std::int32_t>(copy_address + 0x1c) != 0 ||
        read_unaligned<std::int32_t>(copy_address + 0x20) != 0 ||
        read_unaligned<std::int32_t>(copy_address + 0x24) != 1280 * 16 ||
        read_unaligned<std::int32_t>(copy_address + 0x28) != 720 * 16) {
        return g_original_native_solid_node(parent, node, command_stream);
    }

    const auto mapped = mgs4::native_hud::expanded_fixed16_x(canvas);
    if (mapped.left < std::numeric_limits<std::int16_t>::min() ||
        mapped.left > std::numeric_limits<std::int16_t>::max() ||
        mapped.right < std::numeric_limits<std::int16_t>::min() ||
        mapped.right > std::numeric_limits<std::int16_t>::max() ||
        mapped.width <= 0 ||
        mapped.width > std::numeric_limits<std::uint16_t>::max()) {
        return g_original_native_solid_node(parent, node, command_stream);
    }

    write_unaligned(copy_address + 0x1c,
                    static_cast<std::int32_t>(mapped.left));
    write_unaligned(copy_address + 0x24,
                    static_cast<std::int32_t>(mapped.width));
    return g_original_native_solid_node(
        parent, reinterpret_cast<std::uintptr_t>(copy.data()), command_stream);
}

void __fastcall hooked_native_layer_traversal(std::uintptr_t parent,
                                               std::uintptr_t layer) {
    const NodeTraversalContext previous = g_node_traversal_context;
    NodeTraversalContext current{};
    current.active = true;
    current.parent = parent;
    current.layer = layer;
    current.item_count = read_unaligned<std::uint16_t>(layer + 0x10);
    current.priority = read_unaligned<std::uint16_t>(layer + 0x14);
    current.flags = read_unaligned<std::uint16_t>(layer + 0x16);
    g_node_traversal_context = current;
    g_original_native_layer_traversal(parent, layer);
    g_node_traversal_context = previous;
}

std::uint32_t find_raw_layer_item(NodeTraversalContext& context,
                                  std::uintptr_t chain,
                                  std::uintptr_t node) {
    if (!context.active || !context.layer || !context.parent)
        return UINT32_MAX;
    const std::uintptr_t items =
        read_unaligned<std::uintptr_t>(context.layer + 0x18);
    const std::uintptr_t node_table =
        read_unaligned<std::uintptr_t>(context.parent + 0x180);
    const std::uint32_t maximum_node_index =
        read_unaligned<std::uint32_t>(context.parent + 0x170);
    if (!items || !node_table || !context.item_count) return UINT32_MAX;

    const auto matches = [&](std::uint32_t item_index) {
        const std::uintptr_t item = items + item_index * 0x10ull;
        if (read_unaligned<std::uintptr_t>(item + 0x08) != chain)
            return false;
        const std::uint16_t node_index =
            read_unaligned<std::uint16_t>(item);
        if (!node_index || node_index > maximum_node_index) return false;
        return read_unaligned<std::uintptr_t>(node_table + node_index * 8ull) ==
            node;
    };

    const std::uint32_t start = context.last_raw_item_index == UINT32_MAX
        ? 0
        : std::min<std::uint32_t>(context.last_raw_item_index + 1,
                                  context.item_count);
    for (std::uint32_t index = start; index < context.item_count; ++index) {
        if (!matches(index)) continue;
        context.last_raw_item_index = index;
        return index;
    }
    for (std::uint32_t index = 0; index < start; ++index) {
        if (!matches(index)) continue;
        context.last_raw_item_index = index;
        return index;
    }
    return UINT32_MAX;
}

std::uintptr_t __fastcall hooked_native_node_dispatcher(
    std::uintptr_t parent, std::uintptr_t chain, std::uintptr_t node,
    std::uintptr_t auxiliary) {
    if (!parent || !node) {
        return g_original_native_node_dispatcher(parent, chain, node,
                                                 auxiliary);
    }
    const std::uint32_t caller =
        to_caller_rva(
            reinterpret_cast<std::uintptr_t>(MGS4_RETURN_ADDRESS()));
    const std::uint32_t resource =
        read_unaligned<std::uint32_t>(parent + 0x1c);
    const std::uint32_t bytes =
        read_unaligned<std::uint32_t>(parent + 0x34);
    const bool target_resource = is_node_probe_target(resource, bytes);
    const bool normal_traversal =
        caller == kNormalLayerTraversalCaller && target_resource &&
        g_node_traversal_context.active &&
        g_node_traversal_context.parent == parent;
    const std::uint32_t raw_item_index = normal_traversal
        ? find_raw_layer_item(g_node_traversal_context, chain, node)
        : UINT32_MAX;

    const std::uint32_t type_flags =
        read_unaligned<std::uint32_t>(node);
    const std::uint32_t node_type = type_flags & 0x0fu;
    constexpr std::uint32_t kCachedChainMask = 0x24000000u;
    const bool cached_or_nested = (type_flags & kCachedChainMask) != 0;

    const DispatcherContext previous_dispatcher = g_dispatcher_context;
    g_dispatcher_context = DispatcherContext{
        true, normal_traversal, parent, node, raw_item_index};

    if (normal_traversal && (cached_or_nested || node_type >= 0x0a)) {
        g_node_traversal_context.transform_known = false;
        g_node_traversal_context.transform_identity = false;
        g_node_traversal_context.identity_seed_key = 0;
        g_node_traversal_context.identity_seed_raw_item = UINT32_MAX;
    }

    const std::uintptr_t result =
        g_original_native_node_dispatcher(parent, chain, node, auxiliary);
    g_dispatcher_context = previous_dispatcher;

    if (normal_traversal) {
        if (cached_or_nested || node_type >= 0x0a) {
            g_node_traversal_context.transform_known = false;
            g_node_traversal_context.transform_identity = false;
            g_node_traversal_context.identity_seed_key = 0;
            g_node_traversal_context.identity_seed_raw_item = UINT32_MAX;
        } else if (node_type == 0 || node_type == 1) {
            const auto x = read_unaligned<std::int16_t>(node + 0x24);
            const auto y = read_unaligned<std::int16_t>(node + 0x26);
            const auto angle = read_unaligned<std::uint16_t>(node + 0x28);
            const auto scale = read_unaligned<std::int16_t>(node + 0x2a);
            const bool identity =
                x == 0 && y == 0 && angle == 0 && scale == 0x100;
            g_node_traversal_context.transform_known = true;
            g_node_traversal_context.transform_identity = identity;
            g_node_traversal_context.identity_seed_key = identity
                ? read_unaligned<std::uint32_t>(node + 0x04)
                : 0;
            g_node_traversal_context.identity_seed_raw_item = identity
                ? raw_item_index
                : UINT32_MAX;
        }
    }
    return result;
}

struct HookSpec {
    std::uintptr_t rva{};
    void* detour{};
    void** original{};
};

void rollback_hooks(const HookSpec* hooks, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        void* target = reinterpret_cast<void*>(g_base + hooks[index].rva);
        MH_QueueDisableHook(target);
    }
    MH_ApplyQueued();
    for (std::size_t index = 0; index < count; ++index) {
        void* target = reinterpret_cast<void*>(g_base + hooks[index].rva);
        MH_DisableHook(target);
        MH_RemoveHook(target);
    }
}

bool create_and_enable_group(const HookSpec* hooks, std::size_t count) {
    std::size_t created = 0;
    for (; created < count; ++created) {
        void* target = reinterpret_cast<void*>(g_base + hooks[created].rva);
        if (MH_CreateHook(target, hooks[created].detour,
                          hooks[created].original) != MH_OK) {
            rollback_hooks(hooks, created);
            return false;
        }
    }
    for (std::size_t index = 0; index < count; ++index) {
        void* target = reinterpret_cast<void*>(g_base + hooks[index].rva);
        if (MH_QueueEnableHook(target) != MH_OK) {
            rollback_hooks(hooks, count);
            return false;
        }
    }
    if (MH_ApplyQueued() != MH_OK) {
        rollback_hooks(hooks, count);
        return false;
    }
    return true;
}

bool install_core_hooks() {
    const HookSpec hooks[] = {
        {kHudLayoutRva, reinterpret_cast<void*>(&hooked_layout),
         reinterpret_cast<void**>(&g_original_layout)},
        {kPhysicalRectEmitterRva,
         reinterpret_cast<void*>(&hooked_physical_rect),
         reinterpret_cast<void**>(&g_original_physical_rect)},
    };
    return create_and_enable_group(hooks, std::size(hooks));
}

bool install_preview_hook() {
    const HookSpec hook{
        kSemanticOwnerRectRva,
        reinterpret_cast<void*>(&hooked_semantic_owner_rect),
        reinterpret_cast<void**>(&g_original_semantic_owner_rect)};
    return create_and_enable_group(&hook, 1);
}

bool install_modal_hook_trio() {
    // These hooks form one classifier. Enabling only a subset could leave the
    // solid handler without the layer/dispatcher provenance it requires.
    // Create in dependency order and publish all three with one queued apply.
    const HookSpec hooks[] = {
        {kNativeLayerTraversalRva,
         reinterpret_cast<void*>(&hooked_native_layer_traversal),
         reinterpret_cast<void**>(&g_original_native_layer_traversal)},
        {kNativeNodeDispatcherRva,
         reinterpret_cast<void*>(&hooked_native_node_dispatcher),
         reinterpret_cast<void**>(&g_original_native_node_dispatcher)},
        {kNativeSolidNodeRva,
         reinterpret_cast<void*>(&hooked_native_solid_node),
         reinterpret_cast<void**>(&g_original_native_solid_node)},
    };
    return create_and_enable_group(hooks, std::size(hooks));
}

DWORD WINAPI install_late_hooks(void*) {
    int preview_state = 0;  // 0 waiting, 1 installed, -1 failed
    int modal_state = 0;
    for (unsigned attempt = 0; attempt < 24000; ++attempt) {
        if (preview_state == 0 &&
            bytes_match(kSemanticOwnerRectRva, kSemanticOwnerRectBytes,
                        sizeof(kSemanticOwnerRectBytes))) {
            preview_state = install_preview_hook() ? 1 : -1;
            log_line(preview_state == 1
                         ? "Inventory-preview hook active."
                         : "WARNING: inventory-preview hook failed; previews "
                           "remain unmodified.");
        }

        if (modal_state == 0 &&
            bytes_match(kNativeLayerTraversalRva,
                        kNativeLayerTraversalBytes,
                        sizeof(kNativeLayerTraversalBytes)) &&
            bytes_match(kNativeNodeDispatcherRva,
                        kNativeNodeDispatcherBytes,
                        sizeof(kNativeNodeDispatcherBytes)) &&
            bytes_match(kNativeSolidNodeRva, kNativeSolidNodeBytes,
                        sizeof(kNativeSolidNodeBytes))) {
            modal_state = install_modal_hook_trio() ? 1 : -1;
            log_line(modal_state == 1
                         ? "Atomic modal hook trio active."
                         : "WARNING: modal hook trio failed and was rolled "
                           "back; modal tints may be clipped.");
        }

        if (preview_state != 0 && modal_state != 0) break;
        Sleep(25);
    }
    if (preview_state == 0) {
        log_line("WARNING: inventory-preview routine never became available; "
                 "previews remain unmodified.");
    }
    if (modal_state == 0) {
        log_line("WARNING: modal routines never became available; modal "
                 "tints may be clipped.");
    }
    return 0;
}

DWORD WINAPI initialize(void*) {
    if (!resolve_paths()) return 0;
    DeleteFileW(g_log_path);
    log_line("---- MGS4 Ultra120 native centered HUD ----");
    if (!GetPrivateProfileIntW(L"NativeHUD", L"Enabled", 0, g_ini_path)) {
        log_line("Disabled in INI; no hook installed.");
        return 0;
    }
    if (duplicate_native_hud_install_present()) {
        log_line("ERROR: duplicate MGS4NativeCenteredHUD.asi files exist in "
                 "game root and scripts directory; no hook installed.");
        return 0;
    }
    g_base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!supported_executable()) {
        log_line("ERROR: unsupported mgs4.exe; no hook installed.");
        return 0;
    }
    if (!wait_for_core_native_code()) {
        log_line("ERROR: native UI functions did not decrypt or match in "
                 "time.");
        return 0;
    }
    load_configuration();
    const MH_STATUS initialized = MH_Initialize();
    if (initialized != MH_OK &&
        initialized != MH_ERROR_ALREADY_INITIALIZED) {
        log_line("ERROR: MinHook initialization failed (%d).", initialized);
        return 0;
    }
    if (!install_core_hooks()) {
        log_line("ERROR: core native hooks failed and were rolled back.");
        MH_Uninitialize();
        return 0;
    }

    HANDLE thread =
        CreateThread(nullptr, 0, install_late_hooks, nullptr, 0, nullptr);
    if (thread) {
        CloseHandle(thread);
    } else {
        log_line("WARNING: late-hook worker could not start; core HUD remains "
                 "active without preview/modal corrections.");
    }
    log_line("Core logical-range HUD hooks active.");
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, initialize, nullptr, 0,
                                     nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
