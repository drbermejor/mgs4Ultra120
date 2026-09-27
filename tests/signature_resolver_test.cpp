#include "patch_resolver.h"
#include "signature_resolver.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

// A PE image mapped section by section into committed memory, so RVAs and
// RIP-relative operands behave as in the game process.
class MappedImage {
public:
    explicit MappedImage(const char* path) {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) return;
        const std::streamsize size = stream.tellg();
        if (size <= 0) return;
        std::vector<std::uint8_t> file(static_cast<std::size_t>(size));
        stream.seekg(0);
        if (!stream.read(reinterpret_cast<char*>(file.data()), size)) return;
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
        if (file.size() < sizeof(IMAGE_DOS_HEADER) ||
            dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
            file.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return;
        auto* base = static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, nt->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE));
        if (!base) return;
        std::memcpy(base, file.data(), nt->OptionalHeader.SizeOfHeaders);
        const auto* section = IMAGE_FIRST_SECTION(nt);
        for (unsigned index = 0; index < nt->FileHeader.NumberOfSections;
             ++index, ++section) {
            const std::size_t raw = section->PointerToRawData;
            const std::size_t bytes = (std::min)(
                static_cast<std::size_t>(section->SizeOfRawData),
                static_cast<std::size_t>(section->Misc.VirtualSize));
            if (raw > file.size() || bytes > file.size() - raw) continue;
            std::memcpy(base + section->VirtualAddress, file.data() + raw,
                        bytes);
        }
        base_ = reinterpret_cast<std::uintptr_t>(base);
    }
    ~MappedImage() {
        if (base_) VirtualFree(reinterpret_cast<void*>(base_), 0, MEM_RELEASE);
    }
    MappedImage(const MappedImage&) = delete;
    MappedImage& operator=(const MappedImage&) = delete;

    std::uintptr_t base() const { return base_; }

private:
    std::uintptr_t base_{};
};

std::uintptr_t rva(std::uintptr_t base, std::uintptr_t address) {
    return address ? address - base : 0;
}

// Reference-profile addresses (Steam 2026-09-11). The resolver must reproduce
// them exactly from signatures, both at their recorded RVAs and by scanning.
bool check_all(const mgs4::resolver::Locator& locator, const char* label) {
    using namespace mgs4::resolver;
    const std::uintptr_t base = locator.base;
    bool okay = true;
    const auto check = [&](const char* name, std::uintptr_t actual,
                           std::uintptr_t expected) {
        const bool match = rva(base, actual) == expected;
        if (!match) {
            std::cerr << label << ": " << name << " resolved to RVA 0x"
                      << std::hex << rva(base, actual) << ", expected 0x"
                      << expected << std::dec << '\n';
        }
        okay &= match;
    };

    ResolutionTargets resolution{};
    okay &= resolve_resolution(locator, &resolution);
    check("width getter", resolution.width_getter, 0x65c240);
    check("height getter", resolution.height_getter, 0x65c230);
    check("resolution setter", resolution.setter, 0x65f250);
    check("render extent", resolution.render_extent, 0x1b00000);
    check("scaled extent", resolution.scaled_extent, 0x22a8d40);
    check("compositor mirror", resolution.compositor_mirror, 0x1dddab4);
    check("getter block", resolution.getter_block, 0x3bd1158);

    RenderExtent extent{};
    okay &= resolve_render_extent(locator, &extent);
    check("shared render extent", extent.width, 0x1b00000);

    check("projection setter", resolve_projection_setter(locator), 0x0e34d0);
    check("cinematic owner", resolve_cinematic_owner(locator), 0x653000);

    CameraTargets camera{};
    okay &= resolve_camera(locator, &camera);
    check("camera builder", camera.builder, 0x0b9b70);
    check("primary FOV route", camera.primary_return, 0x0ba363);
    check("cinematic source route", camera.cinematic_source_return, 0x0b9b60);
    check("cinematic final route", camera.cinematic_final_return, 0x0eb1ab);

    ControllerTargets controller{};
    okay &= resolve_controller(locator, &controller);
    check("controller setter", controller.setter, 0x7511c0);
    check("controller mask", controller.connection_mask, 0x23d2dc10);

    std::array<std::uintptr_t, 4> sites{};
    okay &= resolve_reticle(locator, resolution.render_extent, &sites);
    check("reticle X1", sites[0], 0xe3a146);
    check("reticle Y1", sites[1], 0xe3a160);
    check("reticle Y2", sites[2], 0xe3a221);
    check("reticle X2", sites[3], 0xe3a23c);

    HudCoreTargets hud{};
    okay &= resolve_hud_core(locator, &hud);
    check("HUD layout", hud.layout, 0x4399e0);
    check("HUD physical rect", hud.physical_rect, 0x0be050);
    check("HUD layout root route", hud.layout_root_return, 0x435f12);
    check("HUD subtitle route", hud.subtitle_return, 0x084b1c);
    check("HUD movie route", hud.movie_return, 0x096c40);
    check("HUD TV movie route", hud.tv_movie_return, 0xe370eb);

    HudPreviewTargets previews{};
    okay &= resolve_hud_previews(locator, &previews);
    check("HUD semantic rect", previews.semantic_rect, 0x4da780);
    check("HUD camouflage route", previews.returns[0], 0x4f8d0b);
    check("HUD item route", previews.returns[1], 0x4fcbb3);
    check("HUD Drebin route", previews.returns[2], 0x504533);
    check("HUD weapon route", previews.returns[3], 0x508773);

    HudModalTargets modal{};
    okay &= resolve_hud_modal(locator, &modal);
    check("HUD layer traversal", modal.layer_traversal, 0x4286e0);
    check("HUD node dispatcher", modal.node_dispatcher, 0x427a80);
    check("HUD solid node", modal.solid_node, 0x4256f0);
    check("HUD normal traversal route", modal.normal_traversal_return,
          0x428813);

    HudMapTargets map{};
    okay &= resolve_hud_map(locator, &map);
    check("HUD map builder", map.builder, 0x4e9ed0);
    check("HUD map init route", map.init_return, 0x4e733f);
    check("HUD map frame route", map.frame_return, 0x4e3f91);
    check("HUD map descriptor", map.descriptor, 0x1c2d270);
    check("HUD map callback", map.callback, 0x4d7bf0);

    HudCodecTargets codec{};
    okay &= resolve_hud_codec(locator, &codec);
    check("HUD aux surface factory", codec.surface_factory, 0x4dbf30);
    check("HUD Codec route", codec.realtime_return, 0x511154);

    HudBriefingTargets briefing{};
    okay &= resolve_hud_briefing(locator, &briefing);
    check("HUD briefing init", briefing.init, 0xe6d850);
    check("HUD briefing child", briefing.child_surface, 0xe7d750);
    check("HUD briefing child route", briefing.child_return, 0xe6ddd2);
    return okay;
}

bool audit_reference_image(const char* path) {
    using namespace mgs4::resolver;
    MappedImage image(path);
    if (!image.base()) {
        std::cerr << "Could not map reference image: " << path << '\n';
        return false;
    }
    const int failures_before = failures;
    const Locator reference = make_locator(image.base(), Mode::reference);
    const Locator relocate = make_locator(image.base(), Mode::relocate);
    expect(check_all(reference, "reference"),
           "every group must resolve at its recorded reference RVAs");
    expect(check_all(relocate, "relocate"),
           "every group must resolve identically by unique .text scan");

    // A single corrupted significant byte must leave its group unresolved in
    // both modes rather than falling back to a guess.
    auto* route = reinterpret_cast<std::uint8_t*>(
        image.base() + mgs4::windows::kCameraPrimaryRoute.reference_rva);
    const std::uint8_t saved = route[0];
    route[0] ^= 0xff;
    CameraTargets unused{};
    expect(!resolve_camera(reference, &unused) &&
               !resolve_camera(relocate, &unused),
           "a corrupted route window must fail the camera group closed");
    route[0] = saved;

    // A duplicated window makes the scan ambiguous: relocation must fail even
    // though the reference RVA still matches.
    const auto& window = mgs4::windows::kProjectionSetter;
    const auto* original = reinterpret_cast<const std::uint8_t*>(
        image.base() + window.reference_rva);
    auto* copy = const_cast<std::uint8_t*>(relocate.text.bytes) +
                 relocate.text.size - window.size - 0x40;
    std::vector<std::uint8_t> backup(copy, copy + window.size);
    std::memcpy(copy, original, window.size);
    expect(resolve_projection_setter(reference) != 0 &&
               resolve_projection_setter(relocate) == 0,
           "a duplicated signature must make relocation fail closed");
    std::memcpy(copy, backup.data(), backup.size());

    if (failures != failures_before) return false;
    std::cout << "reference image: every resolver group reproduced the "
                 "reference profile in both modes\n";
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace mgs4::signature;

    const std::array<std::uint8_t, 14> bytes{
        0x90, 0x48, 0x8b, 0x01, 0xe8, 0x11, 0x22,
        0x33, 0x44, 0xc3, 0x90, 0x90, 0x90, 0x90};
    const auto base = reinterpret_cast<std::uintptr_t>(bytes.data());
    const Region region{bytes.data(), bytes.size(), base};

    const std::array<std::uint8_t, 4> exact_bytes{0x48, 0x8b, 0x01, 0xe8};
    const BytePattern exact{exact_bytes.data(), nullptr, exact_bytes.size()};
    const MatchResult one = find_unique(region, exact);
    expect(one.state == MatchState::unique && one.address == base + 1 &&
               one.count == 1,
           "exact pattern must resolve its unique address");

    const std::array<std::uint8_t, 5> wildcard_bytes{
        0xe8, 0x00, 0x00, 0x00, 0x00};
    const std::array<std::uint8_t, 5> wildcard_mask{1, 0, 0, 0, 0};
    const BytePattern wildcard{wildcard_bytes.data(), wildcard_mask.data(),
                               wildcard_bytes.size()};
    const MatchResult masked = find_unique(region, wildcard);
    expect(masked.state == MatchState::unique && masked.address == base + 4,
           "wildcards must ignore relocation-dependent bytes");

    const std::array<std::uint8_t, 3> leading_bytes{0x00, 0x00, 0xc3};
    const std::array<std::uint8_t, 3> leading_mask{0, 0, 1};
    const BytePattern leading{leading_bytes.data(), leading_mask.data(),
                              leading_bytes.size()};
    const MatchResult anchored = find_unique(region, leading);
    expect(anchored.state == MatchState::unique &&
               anchored.address == base + 7,
           "a leading wildcard must not shift the anchored match");

    const std::array<std::uint8_t, 1> nop_bytes{0x90};
    const BytePattern nop{nop_bytes.data(), nullptr, nop_bytes.size()};
    const MatchResult many = find_unique(region, nop);
    expect(many.state == MatchState::ambiguous && many.address == 0 &&
               many.count == 2,
           "ambiguous patterns must never choose the first match");

    const std::array<std::uint8_t, 2> absent_bytes{0xcc, 0xcc};
    const BytePattern absent{absent_bytes.data(), nullptr,
                             absent_bytes.size()};
    expect(find_unique(region, absent).state == MatchState::none,
           "missing patterns must remain unresolved");

    std::array<std::uint8_t, 8> relative{};
    relative[0] = 0xe8;
    const std::int32_t displacement = 0x1234;
    std::memcpy(relative.data() + 1, &displacement, sizeof(displacement));
    std::uintptr_t target = 0;
    const auto instruction = reinterpret_cast<std::uintptr_t>(relative.data());
    expect(decode_relative_target(instruction, 1, 5, &target) &&
               target == instruction + 5 + displacement,
           "rel32 target must be relative to the instruction end");
    expect(!decode_relative_target(0, 1, 5, &target) &&
               !decode_relative_target(instruction, 4, 5, &target),
           "invalid relative operands must be rejected");

    // Call-route verification and getter decoding on synthetic code.
    {
        using namespace mgs4::resolver;
        std::array<std::uint8_t, 32> code{};
        const auto text = reinterpret_cast<std::uintptr_t>(code.data());
        const Locator locator{text, {code.data(), code.size(), text},
                              Mode::reference};
        code[0] = 0xe8;  // call rel32 +8 -> text + 13
        const std::int32_t call = 8;
        std::memcpy(code.data() + 1, &call, sizeof(call));
        const mgs4::windows::Field field{0, 1, 5};
        expect(verified_return(text, field, text + 13) == text + 5,
               "a route must return after a call to the resolved callee");
        expect(verified_return(text, field, text + 14) == 0,
               "a route calling another function must be rejected");

        const std::uint8_t getter[] = {0x8b, 0x05, 0x10, 0, 0, 0, 0xc3};
        std::memcpy(code.data() + 16, getter, sizeof(getter));
        std::uintptr_t data = 0;
        expect(decode_getter(locator, text + 16, &data) &&
                   data == text + 16 + 6 + 0x10,
               "a trivial getter must decode its RIP-relative data");
        code[22] = 0x90;
        expect(!decode_getter(locator, text + 16, &data),
               "a getter without its return must be rejected");
    }

    const Region process_text = mapped_image_section(
        reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)), ".text");
    expect(process_text.valid(),
           "the resolver must locate the current process .text section");

    // Optional maintainer mode. The executable is supplied at runtime and
    // never added to the repository. SteamStub encrypts the on-disk .text, so
    // this expects an unpacked copy of the reference executable.
    if (argc == 2)
        expect(audit_reference_image(argv[1]),
               "the reference image must reproduce every profile address");

    if (failures) return 1;
    std::cout << "signature resolver tests passed\n";
    return 0;
}
