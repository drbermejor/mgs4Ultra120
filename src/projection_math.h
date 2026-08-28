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

inline bool has_perspective_structure(const float* matrix) {
    if (!matrix) return false;
    return near_zero(matrix[1]) && near_zero(matrix[2]) &&
        near_zero(matrix[3]) && near_zero(matrix[4]) &&
        near_zero(matrix[6]) && near_zero(matrix[7]) &&
        near_zero(matrix[8]) && near_zero(matrix[9]) &&
        near_zero(matrix[12]) && near_zero(matrix[13]) &&
        near_zero(matrix[15]) && std::isfinite(matrix[10]) &&
        std::isfinite(matrix[14]) &&
        std::fabs(std::fabs(matrix[11]) - 1.0f) < 0.0002f;
}

inline bool has_perspective_shape_with_limits(const float* matrix,
                                              float maximum_x,
                                              float maximum_y) {
    if (!matrix) return false;
    const float x = matrix[0];
    const float y = matrix[5];
    const bool finite_scale = std::isfinite(x) && std::isfinite(y) &&
        std::fabs(x) >= 0.25f && std::fabs(x) <= maximum_x &&
        std::fabs(y) >= 0.25f && std::fabs(y) <= maximum_y;
    return finite_scale && has_perspective_structure(matrix);
}

inline bool has_perspective_shape(const float* matrix) {
    return has_perspective_shape_with_limits(matrix, 8.0f, 12.0f);
}

// The central camera builder is positively identified and produces only real
// camera projection variants. MGS4's close-up/collision transitions can raise
// m11 beyond the conservative renderer-wide range. Keep that global limit
// strict, but validate this known camera path by its complete matrix structure
// instead of imposing another arbitrary maximum that a tighter shot may cross.
inline bool has_camera_perspective_shape(const float* matrix) {
    if (!matrix) return false;
    constexpr float minimum_nonzero_scale = 0.00001f;
    return std::isfinite(matrix[0]) && std::isfinite(matrix[5]) &&
        std::fabs(matrix[0]) >= minimum_nonzero_scale &&
        std::fabs(matrix[5]) >= minimum_nonzero_scale &&
        has_perspective_structure(matrix);
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

inline AspectKind classify_camera_aspect(const float* matrix,
                                         float target_aspect) {
    if (!has_camera_perspective_shape(matrix) ||
        !std::isfinite(target_aspect) || target_aspect <= 0.0f) {
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

inline bool adjust_camera_projection(float* matrix, float target_aspect,
                                     float fov_multiplier) {
    if (classify_camera_aspect(matrix, target_aspect) == AspectKind::unknown ||
        !std::isfinite(fov_multiplier) || fov_multiplier <= 0.0f) {
        return false;
    }
    const float original_x = matrix[0];
    const float original_y = matrix[5];
    const float adjusted_y =
        std::copysign(std::fabs(original_y) / fov_multiplier, original_y);
    const float adjusted_x =
        std::copysign(std::fabs(adjusted_y) / target_aspect, original_x);
    if (!std::isfinite(adjusted_x) || !std::isfinite(adjusted_y)) return false;
    matrix[5] = adjusted_y;
    matrix[0] = adjusted_x;
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
