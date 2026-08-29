#pragma once

#include <cmath>
#include <cstdint>

namespace mgs4_hud {

enum class UIRouteMode : std::uint32_t {
    Conservative = 0,
    FullKnownUI = 1,
    Selective = 2,
};

enum class UIPixelKind : std::uint32_t { Unknown, Text, Image, OtherExact };
enum class UIRouteAnchor : std::uint32_t { Unknown, FullScreen, Left, Center, Right };
enum class UIGeometryKind : std::uint32_t {
    Invalid,
    Ordinary,
    LongHorizontal,
    TallVertical,
    LargePanel,
    HighCount,
};

enum UIRouteMask : std::uint32_t {
    kRouteText = 1u << 0,
    kRouteImage = 1u << 1,
    kRouteOrdinary = 1u << 2,
    kRouteLongHorizontal = 1u << 3,
    kRouteTallVertical = 1u << 4,
    kRouteLargePanel = 1u << 5,
    kRouteHighCount = 1u << 6,
    kRouteLeft = 1u << 7,
    kRouteCenter = 1u << 8,
    kRouteRight = 1u << 9,
    kRouteKnown = (1u << 10) - 1u,
    kRouteOtherExact = 1u << 10,
    kRouteUnclassifiedText = 1u << 11,
    kRouteAll = (1u << 12) - 1u,
};

inline UIGeometryKind classify_ui_geometry(std::uint32_t element_count,
                                           float width, float height) {
    if (!element_count || !std::isfinite(width) || !std::isfinite(height) ||
        width <= 0.0f || height <= 0.0f)
        return UIGeometryKind::Invalid;
    if (element_count > 1536u) return UIGeometryKind::HighCount;
    if (element_count == 6u && width >= 512.0f && height <= 180.0f)
        return UIGeometryKind::LongHorizontal;
    if (element_count == 6u && height >= 360.0f && width <= 240.0f)
        return UIGeometryKind::TallVertical;
    if (element_count == 6u &&
        (width >= 512.0f || height >= 360.0f || width * height >= 19600.0f))
        return UIGeometryKind::LargePanel;
    return UIGeometryKind::Ordinary;
}

inline std::uint32_t pixel_mask(UIPixelKind pixel) {
    if (pixel == UIPixelKind::Text) return kRouteText;
    if (pixel == UIPixelKind::Image) return kRouteImage;
    if (pixel == UIPixelKind::OtherExact) return kRouteOtherExact;
    return 0;
}

inline std::uint32_t geometry_mask(UIGeometryKind geometry) {
    switch (geometry) {
    case UIGeometryKind::Ordinary: return kRouteOrdinary;
    case UIGeometryKind::LongHorizontal: return kRouteLongHorizontal;
    case UIGeometryKind::TallVertical: return kRouteTallVertical;
    case UIGeometryKind::LargePanel: return kRouteLargePanel;
    case UIGeometryKind::HighCount: return kRouteHighCount;
    default: return 0;
    }
}

inline std::uint32_t anchor_mask(UIRouteAnchor anchor) {
    if (anchor == UIRouteAnchor::Left) return kRouteLeft;
    if (anchor == UIRouteAnchor::Center) return kRouteCenter;
    if (anchor == UIRouteAnchor::Right) return kRouteRight;
    return 0;
}

inline bool route_ui_draw(UIRouteMode mode, std::uint32_t enabled_mask,
                          UIPixelKind pixel, UIRouteAnchor anchor,
                          std::uint32_t element_count, float width,
                          float height) {
    // Full-screen and unknown draws are never transformed by this experiment.
    if (pixel == UIPixelKind::Unknown || anchor == UIRouteAnchor::Unknown ||
        anchor == UIRouteAnchor::FullScreen || !element_count ||
        element_count > 8192u)
        return false;

    const UIGeometryKind geometry =
        classify_ui_geometry(element_count, width, height);
    if (geometry == UIGeometryKind::Invalid) return false;

    if (mode == UIRouteMode::Conservative) {
        if (pixel == UIPixelKind::OtherExact) return false;
        if (element_count > 1536u) return false;
        return !(element_count == 6u &&
            (width >= 512.0f || height >= 360.0f ||
             width * height >= 19600.0f));
    }
    if (mode == UIRouteMode::FullKnownUI)
        return pixel != UIPixelKind::OtherExact;

    const std::uint32_t required = pixel_mask(pixel) |
        geometry_mask(geometry) | anchor_mask(anchor);
    return required != 0 && (enabled_mask & required) == required;
}

}  // namespace mgs4_hud
