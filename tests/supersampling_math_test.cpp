#include "supersampling_math.h"

#include <cmath>
#include <cstdint>
#include <limits>

int main() {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    if (!mgs4_supersampling::compute_render_extent(
            3440, 1440, 1.5f, &width, &height) ||
        width != 5160 || height != 2160) return 1;
    if (!mgs4_supersampling::compute_render_extent(
            2560, 1080, 2.0f, &width, &height) ||
        width != 5120 || height != 2160) return 2;
    if (!mgs4_supersampling::compute_render_extent(
            3440, 1440, 1.0f, &width, &height) ||
        width != 3440 || height != 1440) return 3;
    if (!mgs4_supersampling::compute_render_extent(
            3, 3, 1.5f, &width, &height) || width != 5 || height != 5) return 4;
    if (mgs4_supersampling::compute_render_extent(
            3440, 1440, 0.99f, &width, &height)) return 5;
    if (mgs4_supersampling::compute_render_extent(
            3440, 1440, std::numeric_limits<float>::infinity(),
            &width, &height)) return 6;
    if (mgs4_supersampling::compute_render_extent(
            std::numeric_limits<std::uint32_t>::max(), 1440, 2.0f,
            &width, &height)) return 7;
    return 0;
}
