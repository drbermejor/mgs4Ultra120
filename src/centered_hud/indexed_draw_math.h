#pragma once

#include <cstdint>
#include <limits>

namespace mgs4_hud {

inline bool index_byte_range(std::uint32_t first_index,
                             std::uint32_t index_count,
                             std::uint32_t index_stride,
                             std::uint64_t buffer_size,
                             std::uint64_t* offset,
                             std::uint64_t* size) {
    if (!index_count || (index_stride != 2 && index_stride != 4) ||
        !offset || !size)
        return false;
    const std::uint64_t begin =
        static_cast<std::uint64_t>(first_index) * index_stride;
    const std::uint64_t bytes =
        static_cast<std::uint64_t>(index_count) * index_stride;
    if (begin > buffer_size || bytes > buffer_size - begin)
        return false;
    *offset = begin;
    *size = bytes;
    return true;
}

inline bool resolve_vertex_index(std::uint32_t raw_index,
                                 std::int32_t base_vertex,
                                 std::uint32_t* resolved) {
    if (!resolved) return false;
    const std::int64_t value = static_cast<std::int64_t>(raw_index) +
        static_cast<std::int64_t>(base_vertex);
    if (value < 0 || value > std::numeric_limits<std::uint32_t>::max())
        return false;
    *resolved = static_cast<std::uint32_t>(value);
    return true;
}

inline bool vertex_byte_range(std::uint32_t minimum_vertex,
                              std::uint32_t maximum_vertex,
                              std::uint32_t stride,
                              std::uint64_t buffer_size,
                              std::uint64_t* offset,
                              std::uint64_t* size) {
    if (!stride || minimum_vertex > maximum_vertex || !offset || !size)
        return false;
    const std::uint64_t begin =
        static_cast<std::uint64_t>(minimum_vertex) * stride;
    const std::uint64_t vertices =
        static_cast<std::uint64_t>(maximum_vertex) - minimum_vertex + 1;
    const std::uint64_t bytes = vertices * stride;
    if (begin > buffer_size || bytes > buffer_size - begin)
        return false;
    *offset = begin;
    *size = bytes;
    return true;
}

}  // namespace mgs4_hud
