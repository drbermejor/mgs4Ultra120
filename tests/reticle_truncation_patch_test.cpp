#include "reticle_truncation_patch.h"

#include <array>
#include <cstddef>

int main() {
    using mgs4_reticle::Axis;
    using mgs4_reticle::PatchSetState;
    using mgs4_reticle::TruncationPatch;
    using mgs4_reticle::truncation_patches;

    if (truncation_patches.size() != 4) return 1;
    if (truncation_patches[0].rva != 0xe39816 ||
        truncation_patches[0].axis != Axis::X ||
        truncation_patches[1].rva != 0xe39830 ||
        truncation_patches[1].axis != Axis::Y ||
        truncation_patches[2].rva != 0xe398f1 ||
        truncation_patches[2].axis != Axis::Y ||
        truncation_patches[3].rva != 0xe3990c ||
        truncation_patches[3].axis != Axis::X) return 2;

    std::array<std::array<unsigned char, 4>, 4> bytes = {};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = truncation_patches[index].expected;

    const auto read = [&](const TruncationPatch& patch) -> const unsigned char* {
        for (std::size_t index = 0; index < truncation_patches.size(); ++index) {
            if (truncation_patches[index].rva == patch.rva)
                return bytes[index].data();
        }
        return static_cast<const unsigned char*>(nullptr);
    };

    if (mgs4_reticle::classify_patch_set(read) != PatchSetState::Original)
        return 3;
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = truncation_patches[index].replacement;
    if (mgs4_reticle::classify_patch_set(read) != PatchSetState::Applied)
        return 4;

    bytes[0] = truncation_patches[0].expected;
    if (mgs4_reticle::classify_patch_set(read) != PatchSetState::Mixed)
        return 5;
    bytes[0][0] = 0xcc;
    if (mgs4_reticle::classify_patch_set(read) != PatchSetState::Unavailable)
        return 6;
    return 0;
}
