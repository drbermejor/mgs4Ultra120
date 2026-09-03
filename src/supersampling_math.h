#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace mgs4_supersampling {

// Pure boundary between user-facing output resolution and the internal render
// extent written to MGS4. The checked double-precision calculation prevents a
// malformed INI value from wrapping the game's 32-bit resolution fields.
inline bool compute_render_extent(std::uint32_t output_width,
                                  std::uint32_t output_height,
                                  float render_scale,
                                  std::uint32_t* render_width,
                                  std::uint32_t* render_height) {
    if (!render_width || !render_height || !output_width || !output_height ||
        !std::isfinite(render_scale) || render_scale < 1.0f) {
        return false;
    }
    const double scaled_width = static_cast<double>(output_width) * render_scale;
    const double scaled_height = static_cast<double>(output_height) * render_scale;
    constexpr double maximum =
        static_cast<double>((std::numeric_limits<std::uint32_t>::max)());
    if (!std::isfinite(scaled_width) || !std::isfinite(scaled_height) ||
        scaled_width > maximum || scaled_height > maximum) {
        return false;
    }
    const double rounded_width = std::floor(scaled_width + 0.5);
    const double rounded_height = std::floor(scaled_height + 0.5);
    if (rounded_width < 1.0 || rounded_height < 1.0) return false;
    *render_width = static_cast<std::uint32_t>(rounded_width);
    *render_height = static_cast<std::uint32_t>(rounded_height);
    return true;
}

}  // namespace mgs4_supersampling
