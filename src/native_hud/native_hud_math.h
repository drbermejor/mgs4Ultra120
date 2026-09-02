#pragma once

#include <cstdint>
#include <limits>

namespace mgs4::native_hud {

struct Canvas {
    std::int32_t output_width{};
    std::int32_t output_height{};
    std::int32_t safe_width{};
    std::int32_t safe_left{};

    constexpr bool active() const {
        return output_width > 0 && output_height > 0 && safe_width > 0 &&
               safe_width < output_width;
    }
};

struct PhysicalRect {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};

    constexpr std::int64_t width() const {
        return static_cast<std::int64_t>(right) - left;
    }
    constexpr std::int64_t height() const {
        return static_cast<std::int64_t>(bottom) - top;
    }
};

struct LogicalRange {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};
};

struct Fixed16HorizontalRange {
    std::int64_t left{};
    std::int64_t width{};
    std::int64_t right{};
};

constexpr Canvas make_canvas(std::int32_t width, std::int32_t height) {
    Canvas result{width, height, width, 0};
    if (width <= 0 || height <= 0) return result;
    const std::int64_t safe = static_cast<std::int64_t>(height) * 16 / 9;
    if (safe <= 0 || safe >= width ||
        safe > std::numeric_limits<std::int32_t>::max())
        return result;
    result.safe_width = static_cast<std::int32_t>(safe);
    result.safe_left = (width - result.safe_width) / 2;
    return result;
}

// These divisions intentionally use integer truncation.  The original game
// converter does the equivalent signed divisions by 1280 and 720.
constexpr std::int32_t logical_x(std::int32_t value, const Canvas& canvas) {
    return canvas.safe_left + static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) * canvas.safe_width / 1280);
}

constexpr std::int32_t logical_width(std::int32_t value,
                                     const Canvas& canvas) {
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) * canvas.safe_width / 1280);
}

constexpr std::int32_t logical_y(std::int32_t value, const Canvas& canvas) {
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) * canvas.output_height / 720);
}

// FUN_140439810 performs these operations in signed 32-bit registers.  The
// native hook deliberately falls back to the original function outside this
// domain instead of relying on C++ signed overflow or emulating wrapped input.
constexpr bool layout_arithmetic_is_safe(std::int32_t x, std::int32_t y,
                                         std::int32_t width,
                                         std::int32_t height,
                                         const Canvas& canvas) {
    const auto fits_i32 = [](std::int64_t value) {
        return value >= std::numeric_limits<std::int32_t>::min() &&
               value <= std::numeric_limits<std::int32_t>::max();
    };
    return fits_i32(static_cast<std::int64_t>(x) + width) &&
           fits_i32(static_cast<std::int64_t>(y) + height) &&
           fits_i32(static_cast<std::int64_t>(x) * canvas.output_width) &&
           fits_i32(static_cast<std::int64_t>(width) * canvas.output_width) &&
           fits_i32(static_cast<std::int64_t>(y) * canvas.output_height) &&
           fits_i32(static_cast<std::int64_t>(height) * canvas.output_height);
}

// Keep a physical viewport at full output size while widening its logical X
// range.  Logical x=0..1280 then lands exactly on the centered safe canvas,
// but the viewport remains capable of drawing output-covering native layers.
constexpr LogicalRange expanded_logical_canvas(const Canvas& canvas) {
    if (!canvas.active()) return {0, 0, 1280, 720};
    const std::int64_t extra =
        static_cast<std::int64_t>(canvas.output_width - canvas.safe_width) *
        640 / canvas.safe_width;
    if (extra < 0 || extra > std::numeric_limits<std::int32_t>::max() - 1280)
        return {0, 0, 1280, 720};
    return {-static_cast<std::int32_t>(extra), 0,
            1280 + static_cast<std::int32_t>(extra), 720};
}

// Native solid-rectangle nodes use 16.4 fixed-point logical coordinates.
// Keep this result wide until the caller has checked the command stream's
// effective signed-X/unsigned-width 16-bit domain.
constexpr Fixed16HorizontalRange expanded_fixed16_x(const Canvas& canvas) {
    const LogicalRange logical = expanded_logical_canvas(canvas);
    const std::int64_t left = static_cast<std::int64_t>(logical.left) * 16;
    const std::int64_t right = static_cast<std::int64_t>(logical.right) * 16;
    return {left, right - left, right};
}

// Expand an already-centered signed 16.4 X coordinate by the inverse of the
// safe-area factor. This is used by auxiliary UI surfaces which accidentally
// receive the centered-HUD horizontal contraction twice. The division rounds
// to nearest, with exact half cases away from zero, so negative and positive
// coordinates remain symmetric around the supplied centre.
constexpr std::int64_t divide_nearest_signed(std::int64_t numerator,
                                             std::int64_t denominator) {
    if (denominator <= 0) return numerator;
    return numerator >= 0
        ? (numerator + denominator / 2) / denominator
        : -((-numerator + denominator / 2) / denominator);
}

constexpr std::int64_t expand_fixed16_x_around(
    std::int32_t value, std::int32_t centre, const Canvas& canvas) {
    if (!canvas.active()) return value;
    return static_cast<std::int64_t>(centre) + divide_nearest_signed(
        (static_cast<std::int64_t>(value) - centre) * canvas.output_width,
        canvas.safe_width);
}

// Re-map a rectangle that is already expressed in output pixels into the
// centered safe canvas.  This is used by native UI producers which bypass the
// game's 1280x720 layout converter (subtitles and movie surfaces).
constexpr std::int32_t physical_x(std::int32_t value,
                                  const Canvas& canvas) {
    if (canvas.output_width <= 0) return value;
    return canvas.safe_left + static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) * canvas.safe_width /
        canvas.output_width);
}

constexpr std::int32_t physical_width(std::int32_t value,
                                      const Canvas& canvas) {
    if (canvas.output_width <= 0) return value;
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) * canvas.safe_width /
        canvas.output_width);
}

// An auxiliary render-target constructor has no X coordinate to remap.  Its
// width still needs exactly the horizontal safe-area factor so its camera and
// viewport retain the original 16:9 aspect.  Invalid inputs are deliberately
// returned unchanged by the caller rather than being coerced here.
constexpr std::int32_t auxiliary_safe_width(std::int32_t value,
                                            const Canvas& canvas) {
    return physical_width(value, canvas);
}

// Fit an already-physical auxiliary render rectangle into the centered safe
// canvas without changing its aspect ratio.  X follows the same affine map as
// the native 2D layout; Y is scaled by the identical factor around the
// rectangle's vertical centre.  Invalid and degenerate rectangles are left
// untouched because the game uses them while constructing/destroying owners.
constexpr PhysicalRect uniform_safe_rect(PhysicalRect rect,
                                         const Canvas& canvas) {
    const std::int64_t width = rect.width();
    const std::int64_t height = rect.height();
    if (!canvas.active() || width <= 0 || height <= 0)
        return rect;

    // The semantic preview producers use in-bounds physical rectangles.  Do
    // not reinterpret sentinels or partially clipped transient rectangles;
    // the original routine must retain ownership of those cases.
    if (rect.left < 0 || rect.top < 0 || rect.right > canvas.output_width ||
        rect.bottom > canvas.output_height)
        return rect;

    const std::int64_t mapped_width =
        width * canvas.safe_width / canvas.output_width;
    const std::int64_t mapped_height =
        height * canvas.safe_width / canvas.output_width;
    if (mapped_width <= 0 || mapped_height <= 0) return rect;

    const std::int64_t mapped_left = static_cast<std::int64_t>(canvas.safe_left) +
        static_cast<std::int64_t>(rect.left) * canvas.safe_width /
            canvas.output_width;
    const std::int64_t mapped_top =
        static_cast<std::int64_t>(rect.top) + (height - mapped_height) / 2;
    const std::int64_t mapped_right = mapped_left + mapped_width;
    const std::int64_t mapped_bottom = mapped_top + mapped_height;
    if (mapped_left < std::numeric_limits<std::int32_t>::min() ||
        mapped_top < std::numeric_limits<std::int32_t>::min() ||
        mapped_right > std::numeric_limits<std::int32_t>::max() ||
        mapped_bottom > std::numeric_limits<std::int32_t>::max())
        return rect;
    return {static_cast<std::int32_t>(mapped_left),
            static_cast<std::int32_t>(mapped_top),
            static_cast<std::int32_t>(mapped_right),
            static_cast<std::int32_t>(mapped_bottom)};
}

}  // namespace mgs4::native_hud
