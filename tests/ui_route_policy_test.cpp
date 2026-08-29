#include "ui_route_policy.h"

#include <cassert>
#include <initializer_list>

using namespace mgs4_hud;

int main() {
    assert(classify_ui_geometry(6, 80.0f, 40.0f) ==
           UIGeometryKind::Ordinary);
    assert(classify_ui_geometry(6, 700.0f, 40.0f) ==
           UIGeometryKind::LongHorizontal);
    assert(classify_ui_geometry(6, 120.0f, 500.0f) ==
           UIGeometryKind::TallVertical);
    assert(classify_ui_geometry(6, 500.0f, 200.0f) ==
           UIGeometryKind::LargePanel);
    assert(classify_ui_geometry(1537, 80.0f, 40.0f) ==
           UIGeometryKind::HighCount);

    // Mode 0 is the exact public alpha.3 policy.
    assert(route_ui_draw(UIRouteMode::Conservative, 0, UIPixelKind::Text,
                         UIRouteAnchor::Left, 60, 100.0f, 30.0f));
    assert(!route_ui_draw(UIRouteMode::Conservative, kRouteAll,
                          UIPixelKind::Image, UIRouteAnchor::Center,
                          6, 512.0f, 20.0f));
    assert(!route_ui_draw(UIRouteMode::Conservative, kRouteAll,
                          UIPixelKind::Image, UIRouteAnchor::Center,
                          1537, 100.0f, 30.0f));

    assert(route_ui_draw(UIRouteMode::FullKnownUI, 0, UIPixelKind::Image,
                         UIRouteAnchor::Right, 6, 800.0f, 500.0f));
    assert(route_ui_draw(UIRouteMode::FullKnownUI, 0, UIPixelKind::Text,
                         UIRouteAnchor::Center, 4096, 100.0f, 30.0f));
    assert(!route_ui_draw(UIRouteMode::FullKnownUI, kRouteAll,
                          UIPixelKind::OtherExact, UIRouteAnchor::Right,
                          6, 200.0f, 100.0f));

    const std::uint32_t text_left_ordinary =
        kRouteText | kRouteOrdinary | kRouteLeft;
    assert(route_ui_draw(UIRouteMode::Selective, text_left_ordinary,
                         UIPixelKind::Text, UIRouteAnchor::Left,
                         60, 100.0f, 30.0f));
    assert(!route_ui_draw(UIRouteMode::Selective, text_left_ordinary,
                          UIPixelKind::Image, UIRouteAnchor::Left,
                          60, 100.0f, 30.0f));
    assert(!route_ui_draw(UIRouteMode::Selective, text_left_ordinary,
                          UIPixelKind::Text, UIRouteAnchor::Right,
                          60, 100.0f, 30.0f));
    const std::uint32_t other_right_ordinary =
        kRouteOtherExact | kRouteOrdinary | kRouteRight;
    assert(route_ui_draw(UIRouteMode::Selective, other_right_ordinary,
                         UIPixelKind::OtherExact, UIRouteAnchor::Right,
                         6, 200.0f, 80.0f));
    assert(!route_ui_draw(UIRouteMode::Selective,
                          kRouteOrdinary | kRouteRight,
                          UIPixelKind::OtherExact, UIRouteAnchor::Right,
                          6, 200.0f, 80.0f));

    // A full-screen draw can never be enabled, even with every bit set.
    for (UIRouteMode mode : {UIRouteMode::Conservative,
                             UIRouteMode::FullKnownUI,
                             UIRouteMode::Selective}) {
        assert(!route_ui_draw(mode, kRouteAll, UIPixelKind::Image,
                              UIRouteAnchor::FullScreen,
                              6, 1280.0f, 720.0f));
    }
    assert(!route_ui_draw(UIRouteMode::FullKnownUI, kRouteAll,
                          UIPixelKind::Unknown, UIRouteAnchor::Center,
                          6, 100.0f, 100.0f));
    return 0;
}
