#include "cache_victim_policy.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using mgs4_hud::prefer_cache_victim;
    using mgs4_hud::shadow_range_fits;
    using mgs4_hud::shadow_write_offset;
    using mgs4_hud::use_full_resource_mirror;

    assert(prefer_cache_victim(false, 20, false, false, 0));
    assert(prefer_cache_victim(false, 20, true, true, 1));
    assert(!prefer_cache_victim(true, 1, true, false, 20));
    assert(prefer_cache_victim(false, 10, true, false, 20));
    assert(!prefer_cache_victim(false, 30, true, false, 20));
    assert(prefer_cache_victim(true, 10, true, true, 20));
    assert(!prefer_cache_victim(true, 30, true, true, 20));

    constexpr std::uint64_t maximum_full_mirror = 16ull * 1024ull * 1024ull;
    assert(!use_full_resource_mirror(false, 4096, maximum_full_mirror));
    assert(!use_full_resource_mirror(true, 0, maximum_full_mirror));
    assert(use_full_resource_mirror(true, maximum_full_mirror,
                                    maximum_full_mirror));
    assert(!use_full_resource_mirror(true, maximum_full_mirror + 1,
                                     maximum_full_mirror));
    assert(shadow_range_fits(128, 96, 1024));
    assert(shadow_range_fits(1024, 0, 1024));
    assert(!shadow_range_fits(1000, 96, 1024));
    assert(!shadow_range_fits(UINT64_MAX - 8, 16, UINT64_MAX));
    assert(shadow_write_offset(false, 4096) == 0);
    assert(shadow_write_offset(true, 4096) == 4096);

    struct Entry {
        bool occupied = false;
        bool ui_relevant = false;
        std::int64_t last_used = 0;
        int identity = -1;
    };
    std::array<Entry, 32> cache{};
    for (int index = 0; index < 4; ++index)
        cache[index] = {true, true, index + 1, index};

    // Simulate far more short-lived renderer buffers than the cache can hold.
    // The four active UI mirrors must survive while expendable entries recycle.
    for (int identity = 4; identity < 10000; ++identity) {
        Entry* victim = nullptr;
        for (Entry& candidate : cache) {
            if (!candidate.occupied) {
                victim = &candidate;
                break;
            }
            if (prefer_cache_victim(
                    candidate.ui_relevant, candidate.last_used,
                    victim != nullptr, victim ? victim->ui_relevant : false,
                    victim ? victim->last_used : 0))
                victim = &candidate;
        }
        assert(victim);
        *victim = {true, false, identity + 1, identity};
    }
    for (int identity = 0; identity < 4; ++identity) {
        bool found = false;
        for (const Entry& entry : cache)
            found |= entry.identity == identity && entry.ui_relevant;
        assert(found);
    }
    return 0;
}
