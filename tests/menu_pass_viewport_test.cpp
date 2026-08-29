#include "menu_pass_viewport.h"

#include <cassert>
#include <cmath>

static bool close(float left, float right) {
    return std::fabs(left - right) < 0.01f;
}

int main() {
    mgs4_hud::PassViewport viewport{0.0f, 0.0f, 3440.0f, 1440.0f};
    mgs4_hud::PassViewport transformed{};
    assert(mgs4_hud::transform_pass_viewport(
        3440.0f, 1440.0f, viewport, &transformed));
    assert(close(transformed.left, 440.0f));
    assert(close(transformed.width, 2560.0f));
    assert(close(transformed.height, 1440.0f));

    viewport = {1720.0f, 100.0f, 860.0f, 700.0f};
    assert(mgs4_hud::transform_pass_viewport(
        3440.0f, 1440.0f, viewport, &transformed));
    assert(close(transformed.left, 1720.0f));
    assert(close(transformed.width, 640.0f));
    assert(close(transformed.top, 100.0f));

    mgs4_hud::PassScissor scissor{0, 0, 3440, 1440};
    mgs4_hud::PassScissor transformed_scissor{};
    assert(mgs4_hud::transform_pass_scissor(
        3440.0f, 1440.0f, scissor, &transformed_scissor));
    assert(transformed_scissor.left == 440);
    assert(transformed_scissor.right == 3000);
    assert(transformed_scissor.top == 0);
    assert(transformed_scissor.bottom == 1440);

    assert(!mgs4_hud::transform_pass_viewport(
        1920.0f, 1080.0f, viewport, &transformed));

    viewport = {1916.0f, 510.0f, 1514.0f, 662.0f};
    assert(mgs4_hud::transform_pass_viewport_preserve_16x9(
        3440.0f, 1440.0f, viewport, &transformed));
    assert(close(transformed.width, 662.0f * (16.0f / 9.0f)));
    assert(close(transformed.height, 662.0f));
    assert(transformed.left >= 440.0f);
    assert(transformed.left + transformed.width <= 3000.01f);

    viewport = {0.0f, 0.0f, 3440.0f, 1440.0f};
    assert(mgs4_hud::transform_pass_viewport_preserve_16x9(
        3440.0f, 1440.0f, viewport, &transformed));
    assert(close(transformed.left, 440.0f));
    assert(close(transformed.width, 2560.0f));

    viewport = {0.0f, 0.0f, 2048.0f, 4096.0f};
    assert(!mgs4_hud::transform_pass_viewport_preserve_16x9(
        3440.0f, 1440.0f, viewport, &transformed));

    const mgs4_hud::PassViewport preview{1916.0f, 510.0f, 1514.0f, 662.0f};
    assert(mgs4_hud::is_preview_viewport_candidate(
        3440.0f, 1440.0f, preview));
    assert(mgs4_hud::transform_preview_viewport_uniform(
        3440.0f, 1440.0f, preview, &transformed));
    assert(close(transformed.left, 1865.8605f));
    assert(close(transformed.top, 594.6744f));
    assert(close(transformed.width, 1126.6976f));
    assert(close(transformed.height, 492.6512f));
    assert(close(transformed.top + transformed.height * 0.5f,
                 preview.top + preview.height * 0.5f));
    assert(transformed.left + transformed.width <= 3000.01f);

    const mgs4_hud::PassViewport full{0.0f, 0.0f, 3440.0f, 1440.0f};
    const mgs4_hud::PassViewport intermediate{0.0f, 0.0f, 2560.0f, 1072.0f};
    const mgs4_hud::PassViewport shadow{0.0f, 0.0f, 2048.0f, 4096.0f};
    assert(!mgs4_hud::is_preview_viewport_candidate(
        3440.0f, 1440.0f, full));
    assert(!mgs4_hud::is_preview_viewport_candidate(
        3440.0f, 1440.0f, intermediate));
    assert(!mgs4_hud::is_preview_viewport_candidate(
        3440.0f, 1440.0f, shadow));

    const float scaled_width = 2560.0f;
    const float scaled_height = 1080.0f;
    const mgs4_hud::PassViewport scaled_preview{
        preview.left * scaled_width / 3440.0f,
        preview.top * scaled_height / 1440.0f,
        preview.width * scaled_width / 3440.0f,
        preview.height * scaled_height / 1440.0f};
    assert(mgs4_hud::is_preview_viewport_candidate(
        scaled_width, scaled_height, scaled_preview));
    assert(mgs4_hud::transform_preview_viewport_uniform(
        scaled_width, scaled_height, scaled_preview, &transformed));
    assert(transformed.left + transformed.width <= 2240.01f);
    assert(!mgs4_hud::is_preview_viewport_candidate(
        1920.0f, 1080.0f, scaled_preview));
    assert(mgs4_hud::should_transform_preview_viewport(true, 1));
    assert(mgs4_hud::should_transform_preview_viewport(true, 0));
    assert(!mgs4_hud::should_transform_preview_viewport(true, -1));
    assert(!mgs4_hud::should_transform_preview_viewport(false, 1));

    assert(mgs4_hud::transform_preview_viewport_uniform(
        3440.0f, 1440.0f, preview, &transformed));
    const mgs4_hud::PassScissor full_scissor{0, 0, 3440, 1440};
    assert(mgs4_hud::transform_preview_scissor_uniform(
        preview, transformed, full_scissor, &transformed_scissor));
    assert(transformed_scissor.left == 1865);
    assert(transformed_scissor.top == 594);
    assert(transformed_scissor.right == 2993);
    assert(transformed_scissor.bottom == 1088);
    return 0;
}
