#pragma once

#include <cstdint>

namespace mgs4_camera {

// FUN_1400ba380 is the first persistent owner in the main camera rebuild
// chain. Routes after it inherit the adjusted scale and must not apply FOV a
// second time. This RVA is valid only for the executable signature already
// enforced by supported_executable().
inline constexpr std::uintptr_t kPrimaryFovReturnRva = 0x0ba3a3;
inline constexpr std::uintptr_t kCinematicSourceReturnRva = 0x0b9ba0;
inline constexpr std::uintptr_t kCinematicFinalRebuildReturnRva = 0x0eb0eb;

inline bool owns_native_fov(std::uintptr_t caller_return_rva) {
    return caller_return_rva == kPrimaryFovReturnRva;
}

inline bool owns_cinematic_source(std::uintptr_t caller_return_rva) {
    return caller_return_rva == kCinematicSourceReturnRva;
}

inline bool owns_cinematic_final_rebuild(
    std::uintptr_t caller_return_rva) {
    return caller_return_rva == kCinematicFinalRebuildReturnRva;
}

inline bool renderer_is_aspect_only(bool native_active,
                                    bool native_requested) {
    // Active native mode has already applied FOV. An explicit opt-out must also
    // preserve vertical FOV. Only a requested-but-unavailable native hook uses
    // the renderer-level FOV fallback.
    return native_active || !native_requested;
}

}  // namespace mgs4_camera
