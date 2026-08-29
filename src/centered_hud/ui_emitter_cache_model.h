#pragma once

#include <cstddef>
#include <cstring>

namespace mgs4_hud {

struct UIEmitterCacheDecision {
    bool block50_changed;
    bool block10_changed;
};

// Mirrors the two memcmp/copy pairs in FUN_14079f2b0 for a type-1 record.
// The blocks contain five vertex-buffer views (0x50) and one index-buffer view
// (0x10), not shader constants. Advancing local copies lets the passive probe
// report exactly which backend binding setter the game is expected to call.
inline UIEmitterCacheDecision evaluate_and_advance_ui_emitter_cache(
    unsigned char (&cache50)[0x50], unsigned char (&cache10)[0x10],
    const unsigned char* record) {
    if (!record) return {false, false};
    const bool changed50 = std::memcmp(cache50, record, 0x50) != 0;
    const bool changed10 = std::memcmp(cache10, record + 0x50, 0x10) != 0;
    if (changed50) std::memcpy(cache50, record, 0x50);
    if (changed10) std::memcpy(cache10, record + 0x50, 0x10);
    return {changed50, changed10};
}

}  // namespace mgs4_hud
