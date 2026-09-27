#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include "patch_windows.h"
#include "signature_resolver.h"

namespace mgs4::resolver {

// How windows are located.
//
// reference: the executable identity matches the reference profile, so every
//            window must match at its recorded RVA. No scan is performed.
// relocate:  unknown executable. Every window must match exactly once in
//            .text; zero or several matches leave the whole group unresolved.
enum class Mode : std::uint8_t {
    reference,
    relocate,
};

struct Locator {
    std::uintptr_t base{};
    signature::Region text{};
    Mode mode{Mode::reference};
};

inline Locator make_locator(std::uintptr_t base, Mode mode) {
    return {base, signature::mapped_image_section(base, ".text"), mode};
}

inline signature::BytePattern pattern_of(const windows::Window& window) {
    return {window.bytes, window.mask, window.size};
}

inline bool in_text(const Locator& locator, std::uintptr_t address,
                    std::size_t size) {
    const auto& text = locator.text;
    return text.valid() && address >= text.address &&
           address - text.address <= text.size &&
           size <= text.size - (address - text.address);
}

inline bool window_at(const Locator& locator, std::uintptr_t address,
                      const windows::Window& window) {
    return in_text(locator, address, window.size) &&
           signature::matches(reinterpret_cast<const std::uint8_t*>(address),
                              window.size, pattern_of(window));
}

// Returns the window start, or 0 when it is missing, ambiguous or not yet
// decrypted.
inline std::uintptr_t locate(const Locator& locator,
                             const windows::Window& window) {
    if (locator.mode == Mode::reference) {
        const std::uintptr_t address = locator.base + window.reference_rva;
        return window_at(locator, address, window) ? address : 0;
    }
    const auto match = signature::find_unique(locator.text,
                                              pattern_of(window));
    return match.state == signature::MatchState::unique ? match.address : 0;
}

// Diagnostic: number of .text matches, independent of the mode.
inline std::size_t count_matches(const Locator& locator,
                                 const windows::Window& window) {
    return signature::find_unique(locator.text, pattern_of(window)).count;
}

inline std::uintptr_t field_address(std::uintptr_t window,
                                    const windows::Field& field) {
    return window + field.offset;
}

inline std::uintptr_t field_target(std::uintptr_t window,
                                   const windows::Field& field) {
    std::uintptr_t target{};
    return signature::decode_relative_target(window + field.offset,
                                             field.displacement, field.size,
                                             &target)
               ? target
               : 0;
}

// Return address of a call field whose target must be `callee`.
inline std::uintptr_t verified_return(std::uintptr_t window,
                                      const windows::Field& call,
                                      std::uintptr_t callee) {
    if (!window || !callee || field_target(window, call) != callee) return 0;
    return window + call.offset + call.size;
}

// A trivial `mov eax, dword ptr [rip + disp32]; ret` getter.
inline bool decode_getter(const Locator& locator, std::uintptr_t getter,
                          std::uintptr_t* data) {
    if (!in_text(locator, getter, 7)) return false;
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(getter);
    if (bytes[0] != 0x8b || bytes[1] != 0x05 || bytes[6] != 0xc3)
        return false;
    return signature::decode_relative_target(getter, 2, 6, data);
}

// Whether a trivial getter for `data` exists within `radius` of `anchor`.
inline bool getter_exists_near(const Locator& locator, std::uintptr_t anchor,
                               std::size_t radius, std::uintptr_t data) {
    const std::uintptr_t first = anchor > radius ? anchor - radius : 0;
    for (std::uintptr_t address = first; address < anchor + radius;
         address += 0x10) {
        std::uintptr_t candidate{};
        if (decode_getter(locator, address, &candidate) && candidate == data)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Shared: render extent written by the startup copy of the compositor getters.

struct RenderExtent {
    std::uintptr_t width{};   // height is width + 4
};

inline bool resolve_render_extent(const Locator& locator, RenderExtent* out) {
    using namespace windows;
    const std::uintptr_t window = locate(locator, kRenderSizeInit);
    if (!window) return false;
    const std::uintptr_t width = field_target(window, kRenderSizeInitWidthStore);
    const std::uintptr_t height =
        field_target(window, kRenderSizeInitHeightStore);
    if (!width || height != width + 4) return false;
    out->width = width;
    return true;
}

// ---------------------------------------------------------------------------
// Core ASI groups.

struct ResolutionTargets {
    std::uintptr_t width_getter{};
    std::uintptr_t height_getter{};
    std::uintptr_t setter{};
    std::uintptr_t render_extent{};      // width/height pair
    std::uintptr_t scaled_extent{};      // base pair, scaled pair at +8
    std::uintptr_t compositor_mirror{};  // pairs at +0 and +0x18
    std::uintptr_t getter_block{};       // five pairs published by getters
};

inline bool resolve_resolution(const Locator& locator,
                               ResolutionTargets* out) {
    using namespace windows;
    ResolutionTargets result{};
    const std::uintptr_t init = locate(locator, kRenderSizeInit);
    if (!init) return false;
    result.render_extent = field_target(init, kRenderSizeInitWidthStore);
    if (!result.render_extent ||
        field_target(init, kRenderSizeInitHeightStore) !=
            result.render_extent + 4)
        return false;

    // The two compositor getters feed the render extent. Their data is an
    // adjacent width/height pair and the last pair of the getter block.
    result.width_getter = field_target(init, kRenderSizeInitWidthGetterCall);
    result.height_getter = field_target(init, kRenderSizeInitHeightGetterCall);
    std::uintptr_t width_data{}, height_data{};
    if (!decode_getter(locator, result.width_getter, &width_data) ||
        !decode_getter(locator, result.height_getter, &height_data) ||
        height_data != width_data + 4)
        return false;
    result.getter_block = width_data - 0x20;
    for (std::uintptr_t pair = 0; pair < 0x20; pair += 8) {
        if (!getter_exists_near(locator, result.width_getter, 0x4000,
                                result.getter_block + pair))
            return false;
    }

    result.setter = locate(locator, kResolutionSetter);
    if (!result.setter) return false;

    const std::uintptr_t scaled = locate(locator, kScaledExtent);
    if (!scaled) return false;
    result.scaled_extent = field_target(scaled, kScaledExtentBaseWidth);
    if (!result.scaled_extent ||
        field_target(scaled, kScaledExtentBaseHeight) !=
            result.scaled_extent + 4 ||
        field_target(scaled, kScaledExtentScaledWidth) !=
            result.scaled_extent + 8 ||
        field_target(scaled, kScaledExtentScaledHeight) !=
            result.scaled_extent + 12)
        return false;

    const std::uintptr_t mirror = locate(locator, kCompositorMirror);
    if (!mirror) return false;
    result.compositor_mirror = field_target(mirror, kCompositorMirrorTarget0);
    const std::uintptr_t source0 = field_target(mirror, kCompositorMirrorSource0);
    if (!result.compositor_mirror || !source0 ||
        field_target(mirror, kCompositorMirrorSource1) != source0 + 4 ||
        field_target(mirror, kCompositorMirrorTarget1) !=
            result.compositor_mirror + 4 ||
        field_target(mirror, kCompositorMirrorSource2) !=
            result.render_extent ||
        field_target(mirror, kCompositorMirrorTarget2) !=
            result.compositor_mirror + 0x18 ||
        field_target(mirror, kCompositorMirrorSource3) !=
            result.render_extent + 4 ||
        field_target(mirror, kCompositorMirrorTarget3) !=
            result.compositor_mirror + 0x1c)
        return false;

    *out = result;
    return true;
}

inline std::uintptr_t resolve_projection_setter(const Locator& locator) {
    return locate(locator, windows::kProjectionSetter);
}

struct CameraTargets {
    std::uintptr_t builder{};
    std::uintptr_t primary_return{};
    std::uintptr_t cinematic_source_return{};
    std::uintptr_t cinematic_final_return{};
};

inline bool resolve_camera(const Locator& locator, CameraTargets* out) {
    using namespace windows;
    CameraTargets result{};
    result.builder = locate(locator, kCameraBuilder);
    if (!result.builder) return false;
    result.primary_return = verified_return(
        locate(locator, kCameraPrimaryRoute), kCameraPrimaryRouteCall,
        result.builder);
    result.cinematic_source_return = verified_return(
        locate(locator, kCameraCinematicSourceRoute),
        kCameraCinematicSourceRouteCall, result.builder);
    result.cinematic_final_return = verified_return(
        locate(locator, kCameraFinalRebuildRoute),
        kCameraFinalRebuildRouteCall, result.builder);
    if (!result.primary_return || !result.cinematic_source_return ||
        !result.cinematic_final_return)
        return false;
    *out = result;
    return true;
}

inline std::uintptr_t resolve_cinematic_owner(const Locator& locator) {
    return locate(locator, windows::kCinematicOwner);
}

struct ControllerTargets {
    std::uintptr_t setter{};
    std::uintptr_t connection_mask{};
};

inline bool resolve_controller(const Locator& locator,
                               ControllerTargets* out) {
    using namespace windows;
    // The setter body has an identical twin. Select it through its caller
    // and then require the body at that exact address.
    const std::uintptr_t caller = locate(locator, kControllerCaller);
    if (!caller) return false;
    const std::uintptr_t setter =
        field_target(caller, kControllerCallerSetterCall);
    if (!window_at(locator, setter, kControllerSetter)) return false;
    const std::uintptr_t mask_window = locate(locator, kControllerMask);
    if (!mask_window) return false;
    const std::uintptr_t mask =
        field_target(mask_window, kControllerMaskMaskLoad);
    if (!mask || field_target(mask_window, kControllerMaskMaskStore) != mask)
        return false;
    out->setter = setter;
    out->connection_mask = mask;
    return true;
}

// Order matches mgs4_reticle::truncation_patches: X, Y, Y, X.
inline bool resolve_reticle(const Locator& locator,
                            std::uintptr_t render_extent,
                            std::array<std::uintptr_t, 4>* sites) {
    using namespace windows;
    const std::uintptr_t a = locate(locator, kReticleA);
    const std::uintptr_t b = locate(locator, kReticleB);
    if (!a || !b || !render_extent) return false;
    // Both routes divide by the same native render extent the startup copy
    // writes. Agreement of four independent references anchors the sites.
    if (field_target(a, kReticleAWidthDivisor) != render_extent ||
        field_target(a, kReticleAHeightDivisor) != render_extent + 4 ||
        field_target(b, kReticleBWidthDivisor) != render_extent ||
        field_target(b, kReticleBHeightDivisor) != render_extent + 4)
        return false;
    *sites = {field_address(a, kReticleAXSite),
              field_address(a, kReticleAYSite),
              field_address(b, kReticleBYSite),
              field_address(b, kReticleBXSite)};
    return true;
}

// ---------------------------------------------------------------------------
// Native centered HUD groups.

struct HudCoreTargets {
    std::uintptr_t layout{};
    std::uintptr_t physical_rect{};
    std::uintptr_t layout_root_return{};
    std::uintptr_t subtitle_return{};
    std::uintptr_t movie_return{};
    std::uintptr_t tv_movie_return{};
};

inline bool resolve_hud_core(const Locator& locator, HudCoreTargets* out) {
    using namespace windows;
    HudCoreTargets result{};
    result.layout = locate(locator, kHudLayout);
    result.physical_rect = locate(locator, kHudPhysicalRect);
    if (!result.layout || !result.physical_rect) return false;
    result.layout_root_return = verified_return(
        locate(locator, kHudRouteLayoutRoot), kHudRouteLayoutRootCall,
        result.layout);
    if (!result.layout_root_return) return false;
    // Producer routes are individually optional: an unresolved route keeps
    // that producer on the game's original mapping.
    result.subtitle_return = verified_return(
        locate(locator, kHudRouteSubtitle), kHudRouteSubtitleCall,
        result.physical_rect);
    result.movie_return = verified_return(
        locate(locator, kHudRouteMovie), kHudRouteMovieCall,
        result.physical_rect);
    result.tv_movie_return = verified_return(
        locate(locator, kHudRouteTvMovie), kHudRouteTvMovieCall,
        result.physical_rect);
    *out = result;
    return true;
}

struct HudPreviewTargets {
    std::uintptr_t semantic_rect{};
    std::array<std::uintptr_t, 4> returns{};  // camouflage, item, Drebin, weapon
};

inline bool resolve_hud_previews(const Locator& locator,
                                 HudPreviewTargets* out) {
    using namespace windows;
    HudPreviewTargets result{};
    result.semantic_rect = locate(locator, kHudSemanticRect);
    if (!result.semantic_rect) return false;
    result.returns = {
        verified_return(locate(locator, kHudRouteCamouflage),
                        kHudRouteCamouflageCall, result.semantic_rect),
        verified_return(locate(locator, kHudRouteItem), kHudRouteItemCall,
                        result.semantic_rect),
        verified_return(locate(locator, kHudRouteDrebin),
                        kHudRouteDrebinCall, result.semantic_rect),
        verified_return(locate(locator, kHudRouteWeapon),
                        kHudRouteWeaponCall, result.semantic_rect),
    };
    for (const std::uintptr_t route : result.returns)
        if (!route) return false;
    *out = result;
    return true;
}

struct HudModalTargets {
    std::uintptr_t layer_traversal{};
    std::uintptr_t node_dispatcher{};
    std::uintptr_t solid_node{};
    std::uintptr_t normal_traversal_return{};
};

inline bool resolve_hud_modal(const Locator& locator, HudModalTargets* out) {
    using namespace windows;
    HudModalTargets result{};
    result.layer_traversal = locate(locator, kHudLayerTraversal);
    result.node_dispatcher = locate(locator, kHudNodeDispatcher);
    result.solid_node = locate(locator, kHudSolidNode);
    if (!result.layer_traversal || !result.node_dispatcher ||
        !result.solid_node)
        return false;
    result.normal_traversal_return = verified_return(
        locate(locator, kHudRouteNormalTraversal),
        kHudRouteNormalTraversalCall, result.node_dispatcher);
    if (!result.normal_traversal_return) return false;
    *out = result;
    return true;
}

struct HudMapTargets {
    std::uintptr_t builder{};
    std::uintptr_t init_return{};
    std::uintptr_t frame_return{};
    std::uintptr_t descriptor{};
    std::uintptr_t callback{};
};

inline bool resolve_hud_map(const Locator& locator, HudMapTargets* out) {
    using namespace windows;
    HudMapTargets result{};
    result.builder = locate(locator, kHudMapBuilder);
    if (!result.builder) return false;
    result.init_return = verified_return(locate(locator, kHudRouteMapInit),
                                         kHudRouteMapInitCall, result.builder);
    result.frame_return = verified_return(
        locate(locator, kHudRouteMapFrame), kHudRouteMapFrameCall,
        result.builder);
    const std::uintptr_t descriptor = locate(locator, kHudMapDescriptor);
    const std::uintptr_t callback = locate(locator, kHudMapCallback);
    if (!result.init_return || !result.frame_return || !descriptor ||
        !callback)
        return false;
    result.descriptor = field_target(descriptor, kHudMapDescriptorLea);
    result.callback = field_target(callback, kHudMapCallbackLea);
    if (!result.descriptor || !in_text(locator, result.callback, 1))
        return false;
    *out = result;
    return true;
}

struct HudCodecTargets {
    std::uintptr_t surface_factory{};
    std::uintptr_t realtime_return{};
};

inline bool resolve_hud_codec(const Locator& locator, HudCodecTargets* out) {
    using namespace windows;
    HudCodecTargets result{};
    result.surface_factory = locate(locator, kHudAuxSurfaceFactory);
    if (!result.surface_factory) return false;
    result.realtime_return = verified_return(
        locate(locator, kHudRouteCodec), kHudRouteCodecCall,
        result.surface_factory);
    if (!result.realtime_return) return false;
    *out = result;
    return true;
}

struct HudBriefingTargets {
    std::uintptr_t init{};
    std::uintptr_t child_surface{};
    std::uintptr_t child_return{};
};

inline bool resolve_hud_briefing(const Locator& locator,
                                 HudBriefingTargets* out) {
    using namespace windows;
    HudBriefingTargets result{};
    result.init = locate(locator, kHudBriefingInit);
    result.child_surface = locate(locator, kHudBriefingChild);
    if (!result.init || !result.child_surface) return false;
    result.child_return = verified_return(
        locate(locator, kHudRouteBriefingChild), kHudRouteBriefingChildCall,
        result.child_surface);
    if (!result.child_return) return false;
    *out = result;
    return true;
}

}  // namespace mgs4::resolver
