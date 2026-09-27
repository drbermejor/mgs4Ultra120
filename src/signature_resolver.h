#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace mgs4::signature {

struct BytePattern {
    const std::uint8_t* bytes{};
    const std::uint8_t* significant{};  // nullptr means every byte is exact.
    std::size_t size{};
};

struct Region {
    const std::uint8_t* bytes{};
    std::size_t size{};
    std::uintptr_t address{};

    constexpr bool valid() const { return bytes && size && address; }
};

enum class MatchState : std::uint8_t {
    none,
    unique,
    ambiguous,
};

struct MatchResult {
    MatchState state{MatchState::none};
    std::uintptr_t address{};
    std::size_t count{};
};

inline bool valid_pattern(const BytePattern& pattern) {
    return pattern.bytes && pattern.size;
}

inline bool matches(const std::uint8_t* candidate, std::size_t available,
                    const BytePattern& pattern) {
    if (!candidate || !valid_pattern(pattern) || available < pattern.size)
        return false;
    for (std::size_t index = 0; index < pattern.size; ++index) {
        if ((!pattern.significant || pattern.significant[index]) &&
            candidate[index] != pattern.bytes[index]) {
            return false;
        }
    }
    return true;
}

// Return all three states explicitly. Choosing the first of several matches
// would make a short or stale signature silently hook the wrong function.
inline MatchResult find_unique(const Region& region,
                               const BytePattern& pattern) {
    if (!region.valid() || !valid_pattern(pattern) ||
        pattern.size > region.size) {
        return {};
    }

    // Advance with memchr on the first significant byte. A full .text scan is
    // then fast enough to run for every window during startup.
    std::size_t anchor = 0;
    while (anchor < pattern.size && pattern.significant &&
           !pattern.significant[anchor])
        ++anchor;
    if (anchor == pattern.size) return {};

    MatchResult result{};
    const std::size_t last = region.size - pattern.size;
    for (std::size_t offset = 0; offset <= last; ++offset) {
        const void* next = std::memchr(region.bytes + offset + anchor,
                                       pattern.bytes[anchor],
                                       last - offset + 1);
        if (!next) break;
        offset = static_cast<std::size_t>(
                     static_cast<const std::uint8_t*>(next) - region.bytes) -
                 anchor;
        if (!matches(region.bytes + offset, region.size - offset, pattern))
            continue;
        ++result.count;
        if (result.count == 1) {
            result.address = region.address + offset;
        } else {
            result.state = MatchState::ambiguous;
            result.address = 0;
            return result;
        }
    }
    if (result.count == 1) result.state = MatchState::unique;
    return result;
}

inline bool section_name_equals(const IMAGE_SECTION_HEADER& section,
                                const char* name) {
    if (!name) return false;
    char actual[IMAGE_SIZEOF_SHORT_NAME + 1]{};
    std::memcpy(actual, section.Name, IMAGE_SIZEOF_SHORT_NAME);
    return std::strncmp(actual, name, IMAGE_SIZEOF_SHORT_NAME) == 0;
}

// Restrict searches to one mapped PE section. Scanning the process address
// space would cross unrelated modules and can touch inaccessible memory.
inline Region mapped_image_section(std::uintptr_t base, const char* name) {
    if (!base) return {};
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return {};
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base + static_cast<std::uintptr_t>(dos->e_lfanew));
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->FileHeader.NumberOfSections == 0 ||
        nt->FileHeader.NumberOfSections > 96) {
        return {};
    }

    const auto* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned index = 0; index < nt->FileHeader.NumberOfSections;
         ++index, ++section) {
        if (!section_name_equals(*section, name)) continue;
        const std::size_t virtual_size = section->Misc.VirtualSize;
        const std::size_t remaining =
            nt->OptionalHeader.SizeOfImage > section->VirtualAddress
                ? nt->OptionalHeader.SizeOfImage - section->VirtualAddress
                : 0;
        const std::size_t bounded_size = (std::min)(virtual_size, remaining);
        if (!bounded_size) return {};
        const std::uintptr_t address = base + section->VirtualAddress;
        return {reinterpret_cast<const std::uint8_t*>(address), bounded_size,
                address};
    }
    return {};
}

enum class ResolutionKind : std::uint8_t {
    unresolved,
    known_rva,
    unique_signature,
    ambiguous_signature,
};

struct Resolution {
    ResolutionKind kind{ResolutionKind::unresolved};
    std::uintptr_t address{};
    std::size_t matches{};
};

// The known address is the zero-cost path for the validated build. If code
// moves but its complete signature remains unique, section scanning recovers
// the new address. A missing or ambiguous signature fails closed.
inline Resolution resolve_known_or_unique(
    const Region& code, std::uintptr_t known_address,
    const BytePattern& pattern) {
    if (known_address && code.valid() && known_address >= code.address &&
        known_address - code.address <= code.size &&
        pattern.size <= code.size - (known_address - code.address) &&
        matches(reinterpret_cast<const std::uint8_t*>(known_address),
                pattern.size, pattern)) {
        return {ResolutionKind::known_rva, known_address, 1};
    }
    const MatchResult result = find_unique(code, pattern);
    if (result.state == MatchState::unique) {
        return {ResolutionKind::unique_signature, result.address,
                result.count};
    }
    if (result.state == MatchState::ambiguous) {
        return {ResolutionKind::ambiguous_signature, 0, result.count};
    }
    return {};
}

// Decode the target of x86-64 rel32 control flow or RIP-relative addressing.
// The displacement begins at displacement_offset and is relative to the end
// of the complete instruction.
inline bool decode_relative_target(std::uintptr_t instruction,
                                   std::size_t displacement_offset,
                                   std::size_t instruction_size,
                                   std::uintptr_t* target) {
    if (!instruction || !target || instruction_size < sizeof(std::int32_t) ||
        displacement_offset > instruction_size - sizeof(std::int32_t)) {
        return false;
    }
    std::int32_t displacement{};
    std::memcpy(&displacement,
                reinterpret_cast<const void*>(instruction +
                                              displacement_offset),
                sizeof(displacement));
    const std::int64_t next = static_cast<std::int64_t>(instruction) +
        static_cast<std::int64_t>(instruction_size);
    const std::int64_t resolved = next + displacement;
    if (resolved < 0 ||
        static_cast<std::uint64_t>(resolved) >
            (std::numeric_limits<std::uintptr_t>::max)()) {
        return false;
    }
    *target = static_cast<std::uintptr_t>(resolved);
    return true;
}

}  // namespace mgs4::signature
