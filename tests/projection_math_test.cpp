#include "projection_math.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

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

    // A close-up animation observed in the supported executable reaches this
    // target-aspect scale. It must remain outside the global renderer filter
    // but inside the positively identified central camera path.
    float close_up_projection[16] = {
        5.6216335f, 0.0f, 0.0f, 0.0f,
        0.0f, -13.4294577f, 0.0f, 0.0f,
        0.0f, 0.0f, -0.0000381f, 1.0f,
        0.0f, 0.0f, 50.0019f, 0.0f,
    };
    if (mgs4_projection::has_perspective_shape(close_up_projection)) return 7;
    if (!mgs4_projection::has_camera_perspective_shape(close_up_projection))
        return 8;
    if (mgs4_projection::classify_camera_aspect(
            close_up_projection, target_aspect) !=
        mgs4_projection::AspectKind::target) return 9;
    if (!mgs4_projection::adjust_camera_projection(
            close_up_projection, target_aspect, 1.15f)) return 10;
    if (!close_enough(close_up_projection[5], -11.6777893f) ||
        !close_enough(std::fabs(close_up_projection[5] /
                               close_up_projection[0]), target_aspect)) return 11;

    // A known camera projection remains valid at scales above any practical
    // test ceiling. The generic renderer filter must still reject it.
    float extreme_close_up_projection[16] = {
        1000.0f / target_aspect, 0.0f, 0.0f, 0.0f,
        0.0f, -1000.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -0.0000381f, 1.0f,
        0.0f, 0.0f, 50.0019f, 0.0f,
    };
    if (mgs4_projection::has_perspective_shape(extreme_close_up_projection))
        return 12;
    if (!mgs4_projection::has_camera_perspective_shape(
            extreme_close_up_projection)) return 13;
    if (!mgs4_projection::adjust_camera_projection(
            extreme_close_up_projection, target_aspect, 1.15f)) return 14;
    if (!std::isfinite(extreme_close_up_projection[0]) ||
        !std::isfinite(extreme_close_up_projection[5]) ||
        !close_enough(std::fabs(extreme_close_up_projection[5] /
                               extreme_close_up_projection[0]),
                      target_aspect)) return 15;

    float invalid_projection[16];
    std::memcpy(invalid_projection, close_up_projection,
                sizeof(invalid_projection));
    invalid_projection[5] = std::numeric_limits<float>::infinity();
    if (mgs4_projection::has_camera_perspective_shape(invalid_projection))
        return 16;

    // Failed arithmetic must be transactional and leave the matrix untouched.
    float overflow_projection[16];
    std::memcpy(overflow_projection, close_up_projection,
                sizeof(overflow_projection));
    const float overflow_x = overflow_projection[0];
    const float overflow_y = overflow_projection[5];
    if (mgs4_projection::adjust_camera_projection(
            overflow_projection, target_aspect,
            std::numeric_limits<float>::denorm_min())) return 17;
    if (overflow_projection[0] != overflow_x ||
        overflow_projection[5] != overflow_y) return 18;

    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    float planes[24] = {};
    if (!mgs4_projection::rebuild_frustum_planes(planes, identity)) return 19;
    const float expected[24] = {
        -1, 0, 0, 1,  1, 0, 0, 1,
         0, 1, 0, 1,  0,-1, 0, 1,
         0, 0,-1, 1,  0, 0, 1, 1,
    };
    for (unsigned i = 0; i < 24; ++i)
        if (!close_enough(planes[i], expected[i])) return 20;
    return 0;
}
