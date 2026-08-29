#pragma once

#include <cstddef>
#include <cstdint>

namespace mgs4_fov {

enum class CameraRouteTestMode : std::uint32_t {
    NativeAll = 0,
    SelectiveExclusion = 1,
    RendererFallback = 2,
};

// Runtime isolation on the supported Steam executable showed that route 03
// (return RVA 0x0ba3a3, FUN_1400ba380) is the primary camera owner. Applying
// FOV here lets the game's downstream rebuild chain carry the adjusted scale
// without multiplying it again at routes 04-07.
inline constexpr std::size_t kNativeFovPrimaryRouteIndex = 2;
inline constexpr std::size_t kNativeFovCinematicWrapperRouteIndex = 1;
inline constexpr std::size_t kNativeFovFirstRebuildRouteIndex = 3;
inline constexpr std::size_t kNativeFovLastRebuildRouteIndex = 6;

inline bool is_cinematic_rebuild_route(std::size_t route_index) {
    return route_index >= kNativeFovFirstRebuildRouteIndex &&
           route_index <= kNativeFovLastRebuildRouteIndex;
}

inline bool should_apply_native_camera_fov(
    bool native_requested, bool route_test_enabled,
    CameraRouteTestMode mode, std::size_t route_index,
    std::size_t known_route_count, std::uint64_t exclusion_mask,
    bool cinematic_owner_context = false) {
    if (!native_requested) return false;
    if (!route_test_enabled) {
        return (route_index == kNativeFovPrimaryRouteIndex ||
                (cinematic_owner_context &&
                 route_index == kNativeFovCinematicWrapperRouteIndex)) &&
               route_index < known_route_count;
    }
    if (mode == CameraRouteTestMode::RendererFallback) return false;
    if (mode != CameraRouteTestMode::SelectiveExclusion) return true;
    if (route_index >= known_route_count || route_index >= 64) return true;
    return (exclusion_mask & (std::uint64_t{1} << route_index)) == 0;
}

inline bool renderer_owns_fov(bool native_hook_active,
                              bool route_test_enabled,
                              CameraRouteTestMode mode) {
    if (!native_hook_active) return true;
    return route_test_enabled &&
           mode == CameraRouteTestMode::RendererFallback;
}

}  // namespace mgs4_fov
