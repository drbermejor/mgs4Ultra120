#pragma once

#include <algorithm>
#include <cmath>

namespace mgs4_hud {

struct PassViewport {
    float left;
    float top;
    float width;
    float height;
};

struct PassScissor {
    long left;
    long top;
    long right;
    long bottom;
};

inline bool transform_pass_viewport(float output_width, float output_height,
                                    const PassViewport& source,
                                    PassViewport* result) {
    if (!result || !std::isfinite(output_width) ||
        !std::isfinite(output_height) || output_width <= 0.0f ||
        output_height <= 0.0f || !std::isfinite(source.left) ||
        !std::isfinite(source.top) || !std::isfinite(source.width) ||
        !std::isfinite(source.height) || source.width <= 0.0f ||
        source.height <= 0.0f)
        return false;
    const float safe_width = output_height * (16.0f / 9.0f);
    if (output_width <= safe_width + 0.5f) return false;
    const float scale = safe_width / output_width;
    const float safe_left = (output_width - safe_width) * 0.5f;
    *result = source;
    result->left = safe_left + source.left * scale;
    result->width = source.width * scale;
    return std::isfinite(result->left) && std::isfinite(result->width) &&
        result->width > 0.0f;
}

inline bool transform_pass_viewport_preserve_16x9(
    float output_width, float output_height, const PassViewport& source,
    PassViewport* result) {
    if (!result || !std::isfinite(output_width) ||
        !std::isfinite(output_height) || output_width <= 0.0f ||
        output_height <= 0.0f || !std::isfinite(source.left) ||
        !std::isfinite(source.top) || !std::isfinite(source.width) ||
        !std::isfinite(source.height) || source.width <= 0.0f ||
        source.height <= 0.0f || source.left < -0.5f || source.top < -0.5f ||
        source.left + source.width > output_width + 0.5f ||
        source.top + source.height > output_height + 0.5f)
        return false;
    const float safe_width = output_height * (16.0f / 9.0f);
    if (output_width <= safe_width + 0.5f) return false;
    const float scale = safe_width / output_width;
    const float safe_left = (output_width - safe_width) * 0.5f;
    const float mapped_center = safe_left +
        (source.left + source.width * 0.5f) * scale;
    const float corrected_width = std::min(
        safe_width, source.height * (16.0f / 9.0f));
    const float maximum_left = safe_left + safe_width - corrected_width;
    *result = source;
    result->left = std::clamp(mapped_center - corrected_width * 0.5f,
                              safe_left, maximum_left);
    result->width = corrected_width;
    return std::isfinite(result->left) && std::isfinite(result->width) &&
        result->width > 0.0f;
}

// Private weapon/item-preview test. The auxiliary 3D preview already has the
// correct perspective and proportions for its own viewport; it is simply too
// large after the surrounding 2D menu is mapped into the centered safe area.
// Scale both axes uniformly, map X with the safe canvas and keep the original
// vertical centre. This deliberately does not alter a projection matrix.
inline bool transform_preview_viewport_uniform(
    float output_width, float output_height, const PassViewport& source,
    PassViewport* result) {
    if (!result || !std::isfinite(output_width) ||
        !std::isfinite(output_height) || output_width <= 0.0f ||
        output_height <= 0.0f || !std::isfinite(source.left) ||
        !std::isfinite(source.top) || !std::isfinite(source.width) ||
        !std::isfinite(source.height) || source.width <= 0.0f ||
        source.height <= 0.0f)
        return false;
    const float safe_width = output_height * (16.0f / 9.0f);
    if (output_width <= safe_width + 0.5f) return false;
    const float scale = safe_width / output_width;
    const float safe_left = (output_width - safe_width) * 0.5f;
    *result = source;
    result->left = safe_left + source.left * scale;
    result->width = source.width * scale;
    result->height = source.height * scale;
    result->top = source.top + (source.height - result->height) * 0.5f;
    return std::isfinite(result->left) && std::isfinite(result->top) &&
        std::isfinite(result->width) && std::isfinite(result->height) &&
        result->width > 0.0f && result->height > 0.0f;
}

// The runtime route number is session-local and its call stack is shared by
// fullscreen and intermediate viewports. Recognize only the measured
// right-side auxiliary-preview layout, using deliberately conservative ratios
// around the 3440x1440 sample (1916,510 1514x662).
inline bool is_preview_viewport_candidate(
    float output_width, float output_height, const PassViewport& source) {
    if (!std::isfinite(output_width) || !std::isfinite(output_height) ||
        output_width <= 0.0f || output_height <= 0.0f ||
        !std::isfinite(source.left) || !std::isfinite(source.top) ||
        !std::isfinite(source.width) || !std::isfinite(source.height) ||
        source.width <= 0.0f || source.height <= 0.0f)
        return false;
    const float safe_width = output_height * (16.0f / 9.0f);
    if (output_width <= safe_width + 0.5f) return false;
    const float left = source.left / output_width;
    const float top = source.top / output_height;
    const float width = source.width / output_width;
    const float height = source.height / output_height;
    const float right = (source.left + source.width) / output_width;
    const float aspect = source.width / source.height;
    if (source.left < -0.5f || source.top < -0.5f ||
        source.left + source.width > output_width + 0.5f ||
        source.top + source.height > output_height + 0.5f)
        return false;
    return left >= 0.50f && left <= 0.62f &&
        top >= 0.28f && top <= 0.42f &&
        width >= 0.38f && width <= 0.50f &&
        height >= 0.38f && height <= 0.55f &&
        right >= 0.96f && right <= 1.001f &&
        aspect >= 2.0f && aspect <= 2.6f;
}

// rtv_state: 1 = output-sized RTV, -1 = a known non-output RTV, 0 = unknown.
// A known mismatch is authoritative; unknown retains the conservative geometry
// fallback for Proton/driver paths where descriptor provenance is unavailable.
inline bool should_transform_preview_viewport(bool geometry_candidate,
                                              int rtv_state) {
    return geometry_candidate && rtv_state >= 0;
}

inline bool transform_preview_scissor_uniform(
    const PassViewport& source_viewport,
    const PassViewport& transformed_viewport,
    const PassScissor& source, PassScissor* result) {
    if (!result || source.right <= source.left || source.bottom <= source.top ||
        source_viewport.width <= 0.0f || source_viewport.height <= 0.0f ||
        transformed_viewport.width <= 0.0f ||
        transformed_viewport.height <= 0.0f)
        return false;
    const float scale_x = transformed_viewport.width / source_viewport.width;
    const float scale_y = transformed_viewport.height / source_viewport.height;
    if (!std::isfinite(scale_x) || !std::isfinite(scale_y) ||
        scale_x <= 0.0f || scale_y <= 0.0f)
        return false;
    const auto map_x = [&](long value) {
        return transformed_viewport.left +
            (static_cast<float>(value) - source_viewport.left) * scale_x;
    };
    const auto map_y = [&](long value) {
        return transformed_viewport.top +
            (static_cast<float>(value) - source_viewport.top) * scale_y;
    };
    const long viewport_left = static_cast<long>(
        std::floor(transformed_viewport.left));
    const long viewport_top = static_cast<long>(
        std::floor(transformed_viewport.top));
    const long viewport_right = static_cast<long>(
        std::ceil(transformed_viewport.left + transformed_viewport.width));
    const long viewport_bottom = static_cast<long>(
        std::ceil(transformed_viewport.top + transformed_viewport.height));
    result->left = std::max(viewport_left,
        static_cast<long>(std::floor(map_x(source.left))));
    result->top = std::max(viewport_top,
        static_cast<long>(std::floor(map_y(source.top))));
    result->right = std::min(viewport_right,
        static_cast<long>(std::ceil(map_x(source.right))));
    result->bottom = std::min(viewport_bottom,
        static_cast<long>(std::ceil(map_y(source.bottom))));
    return result->right > result->left && result->bottom > result->top;
}

inline bool transform_pass_scissor(float output_width, float output_height,
                                   const PassScissor& source,
                                   PassScissor* result) {
    if (!result || !std::isfinite(output_width) ||
        !std::isfinite(output_height) || output_width <= 0.0f ||
        output_height <= 0.0f || source.right <= source.left ||
        source.bottom <= source.top)
        return false;
    const float safe_width = output_height * (16.0f / 9.0f);
    if (output_width <= safe_width + 0.5f) return false;
    const float scale = safe_width / output_width;
    const float safe_left = (output_width - safe_width) * 0.5f;
    const long minimum = static_cast<long>(std::floor(safe_left));
    const long maximum = static_cast<long>(std::ceil(safe_left + safe_width));
    *result = source;
    result->left = std::max(minimum, static_cast<long>(std::lround(
        safe_left + static_cast<float>(source.left) * scale)));
    result->right = std::min(maximum, static_cast<long>(std::lround(
        safe_left + static_cast<float>(source.right) * scale)));
    return result->right > result->left;
}

}  // namespace mgs4_hud
