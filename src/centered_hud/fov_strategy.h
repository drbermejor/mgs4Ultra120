#pragma once

#include <cmath>

namespace mgs4_fov {

inline bool valid_scale(float value) {
    return std::isfinite(value) && value > 0.0f;
}

// FUN_1400b9bb0 passes this value to the engine projection constructor as the
// common X/Y projection scale. The resulting m00 and m11 are linear in it.
// Dividing here is therefore equivalent to the old m00/m11 FOV adjustment,
// while allowing the original function to rebuild all dependent matrices and
// culling planes itself.
inline float adjusted_camera_scale(float original, float multiplier) {
    if (!valid_scale(original) || !valid_scale(multiplier)) return original;
    const float adjusted = original / multiplier;
    return valid_scale(adjusted) ? adjusted : original;
}

// A diagnostic hook may observe the shared camera builder without owning FOV.
// Keep that mode strictly passive so the renderer-level fallback remains the
// only FOV writer during the A/B test.
inline float camera_scale_for_mode(float original, float multiplier,
                                   bool apply_native_fov) {
    return apply_native_fov
        ? adjusted_camera_scale(original, multiplier)
        : original;
}

// The final renderer hook owns aspect correction only in native-camera mode.
// It deliberately leaves m11 untouched so auxiliary projections do not receive
// the gameplay FOV multiplier and the camera FOV is never applied twice.
inline bool correct_aspect_only(float* matrix, float target_aspect) {
    if (!matrix || !valid_scale(target_aspect) ||
        !std::isfinite(matrix[0]) || !std::isfinite(matrix[5]) ||
        std::fabs(matrix[0]) < 0.00001f || std::fabs(matrix[5]) < 0.00001f) {
        return false;
    }
    const float corrected = std::fabs(matrix[5]) / target_aspect;
    if (!std::isfinite(corrected)) return false;
    matrix[0] = std::copysign(corrected, matrix[0]);
    return true;
}

}  // namespace mgs4_fov
