#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mgs4_reticle {

enum class Axis : std::uint8_t {
    X,
    Y,
};

struct TruncationPatch {
    std::uintptr_t rva;
    std::array<unsigned char, 4> expected;
    std::array<unsigned char, 4> replacement;
    std::size_t size;
    Axis axis;
};

inline constexpr std::array<TruncationPatch, 4> truncation_patches = {{
    {0xe39816,
     {0x0f, 0xbf, 0xd1, 0x00},              // movsx edx, cx
     {0x8b, 0xd1, 0x90, 0x00},              // mov edx, ecx ; nop
     3, Axis::X},
    {0xe39830,
     {0x44, 0x0f, 0xbf, 0xc1},              // movsx r8d, cx
     {0x41, 0x89, 0xc8, 0x90},              // mov r8d, ecx ; nop
     4, Axis::Y},
    {0xe398f1,
     {0x0f, 0xbf, 0xd1, 0x00},              // movsx edx, cx
     {0x8b, 0xd1, 0x90, 0x00},              // mov edx, ecx ; nop
     3, Axis::Y},
    {0xe3990c,
     {0x0f, 0xbf, 0xd1, 0x00},              // movsx edx, cx
     {0x8b, 0xd1, 0x90, 0x00},              // mov edx, ecx ; nop
     3, Axis::X},
}};

inline bool bytes_equal(const unsigned char* actual,
                        const std::array<unsigned char, 4>& expected,
                        std::size_t size) {
    if (!actual || size > expected.size()) return false;
    for (std::size_t index = 0; index < size; ++index) {
        if (actual[index] != expected[index]) return false;
    }
    return true;
}

enum class PatchSetState : std::uint8_t {
    Original,
    Applied,
    Mixed,
    Unavailable,
};

template <typename Reader>
inline PatchSetState classify_patch_set(Reader reader) {
    bool any_original = false;
    bool any_applied = false;
    bool any_unknown = false;
    for (const TruncationPatch& patch : truncation_patches) {
        const unsigned char* bytes = reader(patch);
        if (bytes_equal(bytes, patch.expected, patch.size)) {
            any_original = true;
        } else if (bytes_equal(bytes, patch.replacement, patch.size)) {
            any_applied = true;
        } else {
            any_unknown = true;
        }
    }
    if (!any_unknown && any_original && !any_applied)
        return PatchSetState::Original;
    if (!any_unknown && any_applied && !any_original)
        return PatchSetState::Applied;
    if (!any_unknown && any_original && any_applied)
        return PatchSetState::Mixed;
    return PatchSetState::Unavailable;
}

}  // namespace mgs4_reticle
