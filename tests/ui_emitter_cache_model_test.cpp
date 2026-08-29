#include "ui_emitter_cache_model.h"

#include <cassert>
#include <cstring>

int main() {
    unsigned char cache50[0x50] = {};
    unsigned char cache10[0x10] = {};
    unsigned char record[0x80] = {};

    auto decision = mgs4_hud::evaluate_and_advance_ui_emitter_cache(
        cache50, cache10, record);
    assert(!decision.block50_changed);
    assert(!decision.block10_changed);

    record[12] = 1;
    decision = mgs4_hud::evaluate_and_advance_ui_emitter_cache(
        cache50, cache10, record);
    assert(decision.block50_changed);
    assert(!decision.block10_changed);
    assert(std::memcmp(cache50, record, 0x50) == 0);

    record[0x50 + 3] = 2;
    decision = mgs4_hud::evaluate_and_advance_ui_emitter_cache(
        cache50, cache10, record);
    assert(!decision.block50_changed);
    assert(decision.block10_changed);
    assert(std::memcmp(cache10, record + 0x50, 0x10) == 0);

    record[0] = 3;
    record[0x50] = 4;
    decision = mgs4_hud::evaluate_and_advance_ui_emitter_cache(
        cache50, cache10, record);
    assert(decision.block50_changed);
    assert(decision.block10_changed);
    return 0;
}
