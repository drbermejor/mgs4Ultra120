#include "indexed_draw_math.h"

#include <cassert>
#include <cstdint>

int main() {
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    assert(mgs4_hud::index_byte_range(3, 6, 2, 64, &offset, &size));
    assert(offset == 6);
    assert(size == 12);
    assert(mgs4_hud::index_byte_range(4, 3, 4, 64, &offset, &size));
    assert(offset == 16);
    assert(size == 12);
    assert(!mgs4_hud::index_byte_range(30, 4, 2, 64, &offset, &size));
    assert(!mgs4_hud::index_byte_range(0, 6, 1, 64, &offset, &size));

    std::uint32_t vertex = 0;
    assert(mgs4_hud::resolve_vertex_index(7, -2, &vertex));
    assert(vertex == 5);
    assert(!mgs4_hud::resolve_vertex_index(1, -2, &vertex));
    assert(!mgs4_hud::resolve_vertex_index(UINT32_MAX, 1, &vertex));

    assert(mgs4_hud::vertex_byte_range(5, 9, 12, 256, &offset, &size));
    assert(offset == 60);
    assert(size == 60);
    assert(!mgs4_hud::vertex_byte_range(9, 5, 12, 256, &offset, &size));
    assert(!mgs4_hud::vertex_byte_range(20, 30, 12, 256, &offset, &size));
    return 0;
}
