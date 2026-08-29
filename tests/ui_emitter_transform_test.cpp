#include "ui_emitter_transform.h"

#include <cassert>
#include <cmath>
#include <limits>

static bool near(float a, float b, float epsilon = 0.00001f) {
    return std::fabs(a - b) <= epsilon;
}

int main() {
    const float scale = mgs4_hud::centered_16x9_clip_scale(3440, 1440);
    assert(near(scale, 2560.0f / 3440.0f));
    assert(near(mgs4_hud::centered_16x9_clip_scale(5120, 2160),
                3840.0f / 5120.0f));
    assert(near(mgs4_hud::centered_16x9_clip_scale(1920, 1080), 1.0f));
    assert(near(mgs4_hud::centered_16x9_clip_scale(1280, 1440), 1.0f));
    assert(near(mgs4_hud::centered_16x9_clip_scale(0, 1440), 1.0f));

    float transform[24] = {};
    transform[12] = 2.0f;
    transform[16] = -0.5f;
    transform[20] = 0.75f;
    transform[13] = 3.0f;
    transform[17] = -2.0f / 720.0f;
    transform[21] = -0.25f;
    assert(!mgs4_hud::transform_uses_logical_1280x720_canvas(transform));
    transform[12] = 2.0f / 1280.0f;
    assert(mgs4_hud::transform_uses_logical_1280x720_canvas(transform));
    mgs4_hud::apply_centered_16x9_clip_x(transform, scale);
    assert(near(transform[12], (2.0f / 1280.0f) * scale));
    assert(near(transform[16], -0.5f * scale));
    assert(near(transform[20], 0.75f * scale));
    assert(near(transform[13], 3.0f));
    assert(near(transform[21], -0.25f));

    float invalid[24] = {};
    invalid[12] = std::numeric_limits<float>::quiet_NaN();
    assert(!mgs4_hud::transform_is_patchable(invalid));
    assert(!mgs4_hud::transform_uses_logical_1280x720_canvas(invalid));

    float physical[24] = {};
    physical[12] = 2.0f / 3440.0f;
    physical[17] = -2.0f / 1440.0f;
    assert(!mgs4_hud::transform_uses_logical_1280x720_canvas(physical));

    float canonical_2d[24] = {};
    canonical_2d[12] = 2.0f / 1280.0f;
    canonical_2d[17] = -2.0f / 720.0f;
    canonical_2d[22] = 1.0f;
    assert(mgs4_hud::transform_is_2d_16x9_affine(canonical_2d));

    float scaled_2d[24] = {};
    scaled_2d[12] = 0.00280761719f;
    scaled_2d[17] = -0.0049913195f;
    scaled_2d[22] = 1.0f;
    assert(mgs4_hud::transform_is_2d_16x9_affine(scaled_2d));

    float rotated_2d[24] = {};
    rotated_2d[12] = 0.00004074175f;
    rotated_2d[13] = -0.00277683348f;
    rotated_2d[16] = -0.00156196882f;
    rotated_2d[17] = -0.00007242978f;
    rotated_2d[22] = 1.0f;
    assert(mgs4_hud::transform_is_2d_16x9_affine(rotated_2d));

    physical[22] = 1.0f;
    assert(!mgs4_hud::transform_is_2d_16x9_affine(physical));
    float square_target[24] = {};
    square_target[12] = 2.0f / 256.0f;
    square_target[17] = -2.0f / 256.0f;
    square_target[22] = 1.0f;
    assert(!mgs4_hud::transform_is_2d_16x9_affine(square_target));
    canonical_2d[15] = 0.01f;
    assert(!mgs4_hud::transform_is_2d_16x9_affine(canonical_2d));
    return 0;
}
