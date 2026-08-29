#pragma once

#include <cmath>
#include <cstdint>

namespace mgs4_hud {

// A 96-byte stride-zero buffer bound at vertex slot 1 contains the six float4
// values consumed as the UI transform. The common-emitter type-1 record holds
// a view pointing to this buffer; the floats are not inline in the record.
// Compressing the complete clip-X affine row around zero preserves the 16:9
// pixel aspect without changing Y.
inline float centered_16x9_clip_scale(std::uint32_t width,
                                      std::uint32_t height) {
    if (!width || !height) return 1.0f;
    const float safe_width = static_cast<float>(height) * (16.0f / 9.0f);
    const float scale = safe_width / static_cast<float>(width);
    return std::isfinite(scale) && scale > 0.0f && scale < 1.0f
        ? scale : 1.0f;
}

inline bool transform_is_patchable(const float* transform) {
    if (!transform) return false;
    return std::isfinite(transform[12]) &&
           std::isfinite(transform[16]) &&
           std::isfinite(transform[20]);
}

inline bool transform_uses_logical_1280x720_canvas(
    const float* transform, float epsilon = 0.000001f) {
    if (!transform || !transform_is_patchable(transform) ||
        !std::isfinite(transform[17]))
        return false;
    return std::fabs(transform[12] - (2.0f / 1280.0f)) <= epsilon &&
           std::fabs(transform[17] - (-2.0f / 720.0f)) <= epsilon;
}

// The passive emitter capture proved that the UI has more than the canonical
// unrotated 1280x720 transform. Scaled sub-canvases and rotated UI retain the
// same 16:9 metric but use different coefficients. In this shader layout the
// 2D affine has no X/Y contribution to Z or W, and its constant homogeneous
// component is stored as t[22]=1, t[23]=0 (not the conventional matrix slot
// one might infer without inspecting the shader).
inline bool transform_is_2d_16x9_affine(
    const float* transform, float structural_epsilon = 0.000001f,
    float aspect_epsilon = 0.0005f) {
    if (!transform || !transform_is_patchable(transform)) return false;
    constexpr int required[] = {13, 14, 15, 17, 18, 19, 22, 23};
    for (int index : required) {
        if (!std::isfinite(transform[index])) return false;
    }
    if (std::fabs(transform[14]) > structural_epsilon ||
        std::fabs(transform[15]) > structural_epsilon ||
        std::fabs(transform[18]) > structural_epsilon ||
        std::fabs(transform[19]) > structural_epsilon ||
        std::fabs(transform[22] - 1.0f) > structural_epsilon ||
        std::fabs(transform[23]) > structural_epsilon)
        return false;

    // Row norms preserve the logical canvas aspect even when the element is
    // rotated: hypot(X clip row) / hypot(Y clip row) == 9/16.
    const float x_norm = std::hypot(transform[12], transform[16]);
    const float y_norm = std::hypot(transform[13], transform[17]);
    if (!std::isfinite(x_norm) || !std::isfinite(y_norm) ||
        x_norm <= structural_epsilon || y_norm <= structural_epsilon)
        return false;
    return std::fabs((x_norm / y_norm) - (9.0f / 16.0f)) <=
        aspect_epsilon;
}

inline void apply_centered_16x9_clip_x(float* transform, float scale) {
    if (!transform || !transform_is_patchable(transform) ||
        !std::isfinite(scale) || scale <= 0.0f || scale >= 1.0f)
        return;
    transform[12] *= scale;
    transform[16] *= scale;
    transform[20] *= scale;
}

}  // namespace mgs4_hud
