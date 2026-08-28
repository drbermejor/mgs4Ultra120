#pragma once

#include <cmath>
#include <cstring>

namespace mgs4_projection {

enum class AspectKind {
    unknown,
    widescreen_16_9,
    target,
};

inline bool near_zero(float value) {
    return std::isfinite(value) && std::fabs(value) < 0.00001f;
}

inline bool has_perspective_shape(const float* matrix) {
    if (!matrix) return false;
    const float x = matrix[0];
    const float y = matrix[5];
    const bool finite_scale = std::isfinite(x) && std::isfinite(y) &&
        std::fabs(x) >= 0.25f && std::fabs(x) <= 8.0f &&
        std::fabs(y) >= 0.25f && std::fabs(y) <= 12.0f;
    return finite_scale && near_zero(matrix[1]) && near_zero(matrix[2]) &&
        near_zero(matrix[3]) && near_zero(matrix[4]) && near_zero(matrix[6]) &&
        near_zero(matrix[7]) && near_zero(matrix[8]) && near_zero(matrix[9]) &&
        near_zero(matrix[12]) && near_zero(matrix[13]) && near_zero(matrix[15]) &&
        std::isfinite(matrix[10]) && std::isfinite(matrix[14]) &&
        std::fabs(std::fabs(matrix[11]) - 1.0f) < 0.0002f;
}

inline AspectKind classify_aspect(const float* matrix, float target_aspect) {
    if (!has_perspective_shape(matrix) || !std::isfinite(target_aspect) ||
        target_aspect <= 0.0f) {
        return AspectKind::unknown;
    }
    const float source_aspect = std::fabs(matrix[5] / matrix[0]);
    if (std::fabs(source_aspect - (16.0f / 9.0f)) < 0.0003f)
        return AspectKind::widescreen_16_9;
    if (std::fabs(source_aspect - target_aspect) < 0.0003f)
        return AspectKind::target;
    return AspectKind::unknown;
}

inline bool adjust_projection(float* matrix, float target_aspect,
                              float fov_multiplier, bool accept_target) {
    const AspectKind kind = classify_aspect(matrix, target_aspect);
    if (kind == AspectKind::unknown ||
        (kind == AspectKind::target && !accept_target)) {
        return false;
    }
    const float original_x = matrix[0];
    const float original_y = matrix[5];
    matrix[5] = std::copysign(std::fabs(original_y) / fov_multiplier, original_y);
    matrix[0] = std::copysign(std::fabs(matrix[5]) / target_aspect, original_x);
    return true;
}

inline bool store_normalized_plane(float* destination, float x, float y,
                                   float z, float w) {
    const float length_squared = x * x + y * y + z * z;
    if (!std::isfinite(length_squared) || length_squared <= 0.0f) return false;
    const float inverse_length = 1.0f / std::sqrt(length_squared);
    destination[0] = x * inverse_length;
    destination[1] = y * inverse_length;
    destination[2] = z * inverse_length;
    destination[3] = w * inverse_length;
    return true;
}

// MGS4 stores matrices by columns here. Its visibility planes are extracted as
// column 3 +/- columns 0, 1 and 2, then normalized using xyz.
inline bool rebuild_frustum_planes(float* planes, const float* combined) {
    if (!planes || !combined) return false;
    float rebuilt[24] = {};
    const int component_offsets[4] = { 0, 4, 8, 12 };
    // Preserve the engine's exact plane order: left, right, bottom, top,
    // near and far. The Y pair is intentionally + then -.
    const int axes[6] = { 0, 0, 1, 1, 2, 2 };
    const float signs[6] = { -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f };
    for (unsigned plane_index = 0; plane_index < 6; ++plane_index) {
        const int axis = axes[plane_index];
        const float sign = signs[plane_index];
        const float x = combined[component_offsets[0] + 3] +
            sign * combined[component_offsets[0] + axis];
        const float y = combined[component_offsets[1] + 3] +
            sign * combined[component_offsets[1] + axis];
        const float z = combined[component_offsets[2] + 3] +
            sign * combined[component_offsets[2] + axis];
        const float w = combined[component_offsets[3] + 3] +
            sign * combined[component_offsets[3] + axis];
        if (!store_normalized_plane(rebuilt + plane_index * 4, x, y, z, w))
            return false;
    }
    std::memcpy(planes, rebuilt, sizeof(rebuilt));
    return true;
}

}  // namespace mgs4_projection
