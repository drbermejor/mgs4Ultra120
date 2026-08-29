#include "hud_anchor_math.h"

#include <cassert>
#include <cmath>

static bool close(float left, float right) {
    return std::fabs(left - right) < 0.01f;
}

int main() {
    mgs4_hud::SafeViewport viewport{};
    assert(mgs4_hud::make_16x9_safe_viewport(
        0.0f, 3440.0f, 1440.0f, mgs4_hud::HorizontalAnchor::Center,
        &viewport));
    assert(close(viewport.left, 440.0f));
    assert(close(viewport.width, 2560.0f));

    assert(mgs4_hud::make_16x9_safe_viewport(
        0.0f, 5120.0f, 2160.0f, mgs4_hud::HorizontalAnchor::Center,
        &viewport));
    assert(close(viewport.left, 640.0f));
    assert(close(viewport.width, 3840.0f));

    assert(!mgs4_hud::make_16x9_safe_viewport(
        0.0f, 1920.0f, 1080.0f, mgs4_hud::HorizontalAnchor::Center,
        &viewport));
    return 0;
}
