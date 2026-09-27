#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>

namespace mgs4::profile {

// Executable identity is deliberately separate from runtime address
// discovery. A signature can relocate a known function, but it cannot prove
// that an unknown game build still has the same semantics.
struct ExecutableIdentity {
    DWORD timestamp{};
    DWORD image_size{};
    const char* name{};
};

inline constexpr ExecutableIdentity kSteam20260911{
    0x6aa36b7c, 0x241be000, "Steam 2026-09-11"};

inline bool matches_loaded_image(std::uintptr_t base,
                                 const ExecutableIdentity& identity) {
    if (!base) return false;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base + static_cast<std::uintptr_t>(dos->e_lfanew));
    return nt->Signature == IMAGE_NT_SIGNATURE &&
           nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
           nt->FileHeader.TimeDateStamp == identity.timestamp &&
           nt->OptionalHeader.SizeOfImage == identity.image_size;
}

}  // namespace mgs4::profile
