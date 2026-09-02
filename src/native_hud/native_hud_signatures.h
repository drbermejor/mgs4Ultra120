#pragma once

#include <cstdint>

namespace mgs4::native_hud {

// Exact native layout signature of the auxiliary Codec canvas.  This is an
// allowlist, not a geometry heuristic: every field must match, so unknown
// layouts keep the normal centered-HUD policy.
inline constexpr std::uint32_t kCodecAuxRootLayoutCaller = 0x00435d42;
inline constexpr std::uint32_t kCodecAuxRootResource = 0x00d41dd6;
inline constexpr std::uint32_t kCodecAuxRootAllocationBytes = 0x000035f0;

// The live Codec renderer owns a separate render target/camera.  The caller
// derives its width and height independently from the physical output, which
// gives the camera an ultrawide aspect before the texture is placed inside the
// 16:9 Codec panel.  Keep this as a full call signature: the factory is shared
// by unrelated auxiliary renderers.
inline constexpr std::uint32_t kCodecRealtimeSurfaceCaller = 0x00510f84;
inline constexpr std::uint32_t kCodecRealtimeSurfaceType = 0x0000000d;
inline constexpr std::uint32_t kCodecRealtimeSurfaceResource = 0x00521d96;

// Mission Briefing's normal UI/ticker tree needs the safe physical viewport,
// not the output-wide root used by ordinary HUD layers.  Its exact identity is
// stable and distinct from the auxiliary camera surfaces corrected elsewhere.
inline constexpr std::uint32_t kMissionBriefingUiRootLayoutCaller = 0x00435d42;
inline constexpr std::uint32_t kMissionBriefingUiRootResource = 0x00122bca;
inline constexpr std::uint32_t kMissionBriefingUiRootAllocationBytes = 0x000058ac;

// The pause map creates two siblings from the same resource with the same
// allocation footprint.  Geometry alone cannot distinguish them.  At layout
// time both callback fields are still null, but the internal-map root is
// linked immediately after this exact small root; the other large root is
// linked after the first large root.  Later the internal-map root receives the
// callback below.  Keep every field in the signature so an unknown object
// always falls back to the ordinary centered-HUD policy.
inline constexpr std::uint32_t kPauseMapRootLayoutCaller = 0x00435d42;
inline constexpr std::uint32_t kPauseMapLargeRootResource = 0x001986a8;
inline constexpr std::uint32_t kPauseMapLargeRootAllocationBytes = 0x0001dbb4;
inline constexpr std::uint32_t kPauseMapSmallRootResource = 0x001997e2;
inline constexpr std::uint32_t kPauseMapSmallRootAllocationBytes = 0x00002c00;
inline constexpr std::uint32_t kPauseMapCallbackRva = 0x004d7a20;

enum class PauseMapLargeRootRole : std::uint8_t {
    Unknown = 0,
    Callback = 1,
    Sibling = 2,
};

constexpr bool is_codec_auxiliary_root_layout(
    std::uint32_t caller, std::uint32_t resource,
    std::uint32_t allocation_bytes, std::int32_t x, std::int32_t y,
    std::int32_t width, std::int32_t height) {
    return caller == kCodecAuxRootLayoutCaller &&
           resource == kCodecAuxRootResource &&
           allocation_bytes == kCodecAuxRootAllocationBytes && x == 0 &&
           y == 0 && width == 1280 && height == 720;
}

constexpr bool is_codec_realtime_surface(
    std::uint32_t caller, std::uint32_t type, std::uint32_t resource) {
    return caller == kCodecRealtimeSurfaceCaller &&
           type == kCodecRealtimeSurfaceType &&
           resource == kCodecRealtimeSurfaceResource;
}

constexpr bool is_mission_briefing_ui_root_layout(
    std::uint32_t caller, std::uint32_t resource,
    std::uint32_t allocation_bytes, std::int32_t x, std::int32_t y,
    std::int32_t width, std::int32_t height) {
    return caller == kMissionBriefingUiRootLayoutCaller &&
           resource == kMissionBriefingUiRootResource &&
           allocation_bytes == kMissionBriefingUiRootAllocationBytes &&
           x == 0 && y == 0 && width == 1280 && height == 720;
}

constexpr bool is_pause_map_large_root_layout(
    std::uint32_t caller, std::uint32_t resource,
    std::uint32_t allocation_bytes, std::int32_t x, std::int32_t y,
    std::int32_t width, std::int32_t height) {
    return caller == kPauseMapRootLayoutCaller &&
           resource == kPauseMapLargeRootResource &&
           allocation_bytes == kPauseMapLargeRootAllocationBytes && x == 0 &&
           y == 0 && width == 1280 && height == 720;
}

constexpr PauseMapLargeRootRole classify_pause_map_large_root_layout(
    std::uint32_t caller, std::uint32_t resource,
    std::uint32_t allocation_bytes, std::int32_t x, std::int32_t y,
    std::int32_t width, std::int32_t height,
    std::uintptr_t callback_value, std::uintptr_t module_base,
    std::uint32_t previous_resource,
    std::uint32_t previous_allocation_bytes) {
    if (!is_pause_map_large_root_layout(caller, resource, allocation_bytes,
                                        x, y, width, height))
        return PauseMapLargeRootRole::Unknown;
    if (module_base != 0 && callback_value >= module_base &&
        callback_value - module_base == kPauseMapCallbackRva)
        return PauseMapLargeRootRole::Callback;
    if (callback_value != 0) return PauseMapLargeRootRole::Unknown;
    if (previous_resource == kPauseMapSmallRootResource &&
        previous_allocation_bytes == kPauseMapSmallRootAllocationBytes)
        return PauseMapLargeRootRole::Callback;
    if (previous_resource == kPauseMapLargeRootResource &&
        previous_allocation_bytes == kPauseMapLargeRootAllocationBytes)
        return PauseMapLargeRootRole::Sibling;
    return PauseMapLargeRootRole::Unknown;
}

}  // namespace mgs4::native_hud
