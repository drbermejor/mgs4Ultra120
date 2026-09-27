#pragma once

#include <cstddef>
#include <cstdint>

namespace mgs4::windows {

// A masked instruction window from the reference executable. The reference
// RVA is the zero-cost location on that build; on any other build the window
// must be found exactly once in .text.
struct Window {
    const char* name{};
    std::uint32_t reference_rva{};
    const std::uint8_t* bytes{};
    const std::uint8_t* mask{};
    std::size_t size{};
};

// One instruction inside a window. For call/RIP-relative fields, the 32-bit
// displacement starts at `displacement` and is relative to the end of the
// `size`-byte instruction. Site fields use only `offset`.
struct Field {
    std::uint16_t offset{};
    std::uint8_t displacement{};
    std::uint8_t size{};
};

// Expected number of .text matches for a signature audit. A body with a
// known native twin is resolved through its caller and expects two.
struct AuditEntry {
    const Window* window{};
    std::size_t expected_matches{};
};

}  // namespace mgs4::windows
