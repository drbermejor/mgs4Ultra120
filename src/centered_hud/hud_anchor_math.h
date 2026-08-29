#pragma once

#include <cmath>

namespace mgs4_hud {

enum class HorizontalAnchor { Left, Center, Right };

struct SafeViewport {
    float left;
    float width;
};

inline bool make_16x9_safe_viewport(float original_left,
                                    float original_width,
                                    float original_height,
                                    HorizontalAnchor anchor,
                                    SafeViewport* result) {
    if (!result || !std::isfinite(original_left) ||
        !std::isfinite(original_width) || !std::isfinite(original_height) ||
        original_width <= 0.0f || original_height <= 0.0f)
        return false;

    const float safe_width = original_height * (16.0f / 9.0f);
    if (!std::isfinite(safe_width) || original_width <= safe_width + 0.5f)
        return false;

    const float extra = original_width - safe_width;
    float offset = 0.0f;
    if (anchor == HorizontalAnchor::Center)
        offset = extra * 0.5f;
    else if (anchor == HorizontalAnchor::Right)
        offset = extra;
    result->left = original_left + offset;
    result->width = safe_width;
    return true;
}

}  // namespace mgs4_hud
