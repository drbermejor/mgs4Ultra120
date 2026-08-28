#include "projection_math.h"

#include <cmath>
#include <cstdio>

static bool close_enough(float actual, float expected, float epsilon = 0.0001f) {
    if (std::fabs(actual - expected) <= epsilon) return true;
    std::fprintf(stderr, "expected %.7f, got %.7f\n", expected, actual);
    return false;
}

int main() {
    constexpr float target_aspect = 3440.0f / 1440.0f;
    float projection[16] = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -3.5555556f, 0.0f, 0.0f,
        0.0f, 0.0f, -0.0000381f, 1.0f,
        0.0f, 0.0f, 50.0019f, 0.0f,
    };
    if (mgs4_projection::classify_aspect(projection, target_aspect) !=
        mgs4_projection::AspectKind::widescreen_16_9) return 1;
    if (!mgs4_projection::adjust_projection(projection, target_aspect, 1.15f, true))
        return 2;
    if (!close_enough(projection[5], -3.0917876f) ||
        !close_enough(projection[0], 1.2942367f)) return 3;
    if (mgs4_projection::classify_aspect(projection, target_aspect) !=
        mgs4_projection::AspectKind::target) return 4;

    // The late fallback must reject an already corrected target-aspect matrix.
    float fallback_copy[16];
    std::memcpy(fallback_copy, projection, sizeof(fallback_copy));
    if (mgs4_projection::adjust_projection(fallback_copy, target_aspect, 1.15f,
                                           false)) return 5;
    if (!close_enough(fallback_copy[0], projection[0]) ||
        !close_enough(fallback_copy[5], projection[5])) return 6;

    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    float planes[24] = {};
    if (!mgs4_projection::rebuild_frustum_planes(planes, identity)) return 7;
    const float expected[24] = {
        -1, 0, 0, 1,  1, 0, 0, 1,
         0, 1, 0, 1,  0,-1, 0, 1,
         0, 0,-1, 1,  0, 0, 1, 1,
    };
    for (unsigned i = 0; i < 24; ++i)
        if (!close_enough(planes[i], expected[i])) return 8;
    return 0;
}
