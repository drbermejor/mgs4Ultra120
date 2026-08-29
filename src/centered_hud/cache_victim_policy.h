#pragma once

#include <cstdint>

namespace mgs4_hud {

// Prefer expendable renderer data over entries already proven to feed the UI.
// Within the same class, recycle the least-recently-used entry.
inline bool prefer_cache_victim(bool candidate_ui_relevant,
                                std::int64_t candidate_last_used,
                                bool current_valid,
                                bool current_ui_relevant,
                                std::int64_t current_last_used) {
    if (!current_valid) return true;
    if (candidate_ui_relevant != current_ui_relevant)
        return !candidate_ui_relevant;
    return candidate_last_used < current_last_used;
}

inline bool use_full_resource_mirror(bool ui_relevant,
                                     std::uint64_t resource_size,
                                     std::uint64_t maximum_size) {
    return ui_relevant && resource_size != 0 &&
        resource_size <= maximum_size;
}

inline bool shadow_range_fits(std::uint64_t begin, std::uint64_t size,
                              std::uint64_t capacity) {
    return begin <= capacity && size <= capacity - begin;
}

inline std::uint64_t shadow_write_offset(bool full_resource,
                                         std::uint64_t destination_offset) {
    return full_resource ? destination_offset : 0;
}

}  // namespace mgs4_hud
