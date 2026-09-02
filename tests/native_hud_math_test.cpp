#include "native_hud_math.h"
#include "native_hud_signatures.h"

#include <cassert>
#include <limits>

int main() {
    using namespace mgs4::native_hud;

    static_assert(is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller + 1, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource + 1,
        kCodecAuxRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes + 1, 0, 0, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 1, 0, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 0, 1, 1280, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 0, 0, 1279, 720));
    static_assert(!is_codec_auxiliary_root_layout(
        kCodecAuxRootLayoutCaller, kCodecAuxRootResource,
        kCodecAuxRootAllocationBytes, 0, 0, 1280, 719));
    static_assert(is_codec_realtime_surface(
        kCodecRealtimeSurfaceCaller, kCodecRealtimeSurfaceType,
        kCodecRealtimeSurfaceResource));
    static_assert(!is_codec_realtime_surface(
        kCodecRealtimeSurfaceCaller + 1, kCodecRealtimeSurfaceType,
        kCodecRealtimeSurfaceResource));
    static_assert(!is_codec_realtime_surface(
        kCodecRealtimeSurfaceCaller, kCodecRealtimeSurfaceType + 1,
        kCodecRealtimeSurfaceResource));
    static_assert(!is_codec_realtime_surface(
        kCodecRealtimeSurfaceCaller, kCodecRealtimeSurfaceType,
        kCodecRealtimeSurfaceResource + 1));
    static_assert(is_mission_briefing_ui_root_layout(
        kMissionBriefingUiRootLayoutCaller,
        kMissionBriefingUiRootResource,
        kMissionBriefingUiRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_mission_briefing_ui_root_layout(
        kMissionBriefingUiRootLayoutCaller,
        kMissionBriefingUiRootResource + 1,
        kMissionBriefingUiRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_mission_briefing_ui_root_layout(
        kMissionBriefingUiRootLayoutCaller + 1,
        kMissionBriefingUiRootResource,
        kMissionBriefingUiRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(!is_mission_briefing_ui_root_layout(
        kMissionBriefingUiRootLayoutCaller,
        kMissionBriefingUiRootResource,
        kMissionBriefingUiRootAllocationBytes + 1, 0, 0, 1280, 720));
    static_assert(!is_mission_briefing_ui_root_layout(
        kMissionBriefingUiRootLayoutCaller,
        kMissionBriefingUiRootResource,
        kMissionBriefingUiRootAllocationBytes, 0, 0, 1279, 720));

    constexpr std::uintptr_t test_base = 0x140000000ull;
    static_assert(is_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720));
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720,
        test_base + kPauseMapCallbackRva,
        test_base, 0, 0) == PauseMapLargeRootRole::Callback);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720, 0,
        test_base, kPauseMapSmallRootResource,
        kPauseMapSmallRootAllocationBytes) ==
        PauseMapLargeRootRole::Callback);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720, 0,
        test_base, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes) ==
        PauseMapLargeRootRole::Sibling);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720, 0,
        test_base, 0, 0) == PauseMapLargeRootRole::Unknown);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720,
        test_base + kPauseMapCallbackRva + 1,
        test_base, kPauseMapSmallRootResource,
        kPauseMapSmallRootAllocationBytes) ==
        PauseMapLargeRootRole::Unknown);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller + 1, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720,
        test_base + kPauseMapCallbackRva,
        test_base, 0, 0) == PauseMapLargeRootRole::Unknown);
    static_assert(classify_pause_map_large_root_layout(
        kPauseMapRootLayoutCaller, kPauseMapLargeRootResource,
        kPauseMapLargeRootAllocationBytes, 0, 0, 1280, 720,
        kPauseMapCallbackRva, 0, kPauseMapSmallRootResource,
        kPauseMapSmallRootAllocationBytes) ==
        PauseMapLargeRootRole::Unknown);

    constexpr Canvas uw = make_canvas(3440, 1440);
    static_assert(uw.active());
    static_assert(uw.safe_width == 2560);
    static_assert(uw.safe_left == 440);
    static_assert(logical_x(0, uw) == 440);
    static_assert(logical_x(640, uw) == 1720);
    static_assert(logical_x(1280, uw) == 3000);
    static_assert(logical_width(1280, uw) == 2560);
    static_assert(logical_y(720, uw) == 1440);
    static_assert(layout_arithmetic_is_safe(0, 0, 1280, 720, uw));
    static_assert(!layout_arithmetic_is_safe(
        std::numeric_limits<std::int32_t>::max(), 0, 1, 1, uw));
    static_assert(!layout_arithmetic_is_safe(700000, 0, 1, 1, uw));
    static_assert(!layout_arithmetic_is_safe(0, 0, 700000, 1, uw));
    static_assert(!layout_arithmetic_is_safe(0, 1600000, 1, 1, uw));
    static_assert(physical_x(0, uw) == 440);
    static_assert(physical_x(1720, uw) == 1720);
    static_assert(physical_x(3440, uw) == 3000);
    static_assert(physical_width(3440, uw) == 2560);
    static_assert(physical_width(1720, uw) == 1280);
    static_assert(auxiliary_safe_width(1671, uw) == 1243);
    constexpr LogicalRange uw_logical = expanded_logical_canvas(uw);
    static_assert(uw_logical.left == -220 && uw_logical.top == 0 &&
                  uw_logical.right == 1500 && uw_logical.bottom == 720);
    static_assert(uw_logical.right - uw_logical.left == 1720);
    constexpr Fixed16HorizontalRange uw_fixed = expanded_fixed16_x(uw);
    static_assert(uw_fixed.left == -3520 && uw_fixed.width == 27520 &&
                  uw_fixed.right == 24000);
    static_assert(divide_nearest_signed(5, 2) == 3);
    static_assert(divide_nearest_signed(-5, 2) == -3);
    static_assert(divide_nearest_signed(4, 2) == 2);
    static_assert(divide_nearest_signed(-4, 2) == -2);
    constexpr std::int32_t map_centre = 13392;
    static_assert(expand_fixed16_x_around(7392, map_centre, uw) == 5329);
    static_assert(expand_fixed16_x_around(9632, map_centre, uw) == 8339);
    static_assert(expand_fixed16_x_around(18592, map_centre, uw) == 20380);
    static_assert(expand_fixed16_x_around(19392, map_centre, uw) == 21455);

    constexpr PhysicalRect weapon =
        uniform_safe_rect({1916, 510, 3431, 1172}, uw);
    static_assert(weapon.left == 1865);
    static_assert(weapon.top == 595);
    static_assert(weapon.right == 2992);
    static_assert(weapon.bottom == 1087);

    constexpr PhysicalRect item =
        uniform_safe_rect({1916, 370, 3431, 1032}, uw);
    static_assert(item.left == 1865);
    static_assert(item.top == 455);
    static_assert(item.right == 2992);
    static_assert(item.bottom == 947);

    constexpr PhysicalRect shop =
        uniform_safe_rect({2128, 502, 3216, 978}, uw);
    static_assert(shop.left == 2023);
    static_assert(shop.top == 563);
    static_assert(shop.right == 2832);
    static_assert(shop.bottom == 917);

    constexpr PhysicalRect camouflage =
        uniform_safe_rect({209, 60, 3238, 1386}, uw);
    static_assert(camouflage.left == 595);
    static_assert(camouflage.top == 230);
    static_assert(camouflage.right == 2849);
    static_assert(camouflage.bottom == 1216);

    constexpr PhysicalRect degenerate =
        uniform_safe_rect({0, 0, 0, 0}, uw);
    static_assert(degenerate.left == 0 && degenerate.top == 0 &&
                  degenerate.right == 0 && degenerate.bottom == 0);
    constexpr PhysicalRect clipped =
        uniform_safe_rect({-10, 20, 110, 220}, uw);
    static_assert(clipped.left == -10 && clipped.top == 20 &&
                  clipped.right == 110 && clipped.bottom == 220);
    constexpr PhysicalRect sentinel = uniform_safe_rect(
        {std::numeric_limits<std::int32_t>::min(), 0,
         std::numeric_limits<std::int32_t>::max(), 100},
        uw);
    static_assert(sentinel.left == std::numeric_limits<std::int32_t>::min() &&
                  sentinel.right == std::numeric_limits<std::int32_t>::max());

    constexpr Canvas wide = make_canvas(5120, 1440);
    static_assert(wide.active());
    static_assert(wide.safe_width == 2560);
    static_assert(wide.safe_left == 1280);
    static_assert(logical_x(0, wide) == 1280);
    static_assert(logical_x(1280, wide) == 3840);
    constexpr LogicalRange wide_logical = expanded_logical_canvas(wide);
    static_assert(wide_logical.left == -640 &&
                  wide_logical.right == 1920);
    static_assert(wide_logical.right - wide_logical.left == 2560);
    constexpr Fixed16HorizontalRange wide_fixed = expanded_fixed16_x(wide);
    static_assert(wide_fixed.left == -10240 && wide_fixed.width == 40960 &&
                  wide_fixed.right == 30720);

    constexpr Canvas native = make_canvas(2560, 1440);
    static_assert(!native.active());
    static_assert(physical_x(123, native) == 123);
    static_assert(physical_width(456, native) == 456);
    constexpr LogicalRange native_logical = expanded_logical_canvas(native);
    static_assert(native_logical.left == 0 && native_logical.right == 1280);
    constexpr Fixed16HorizontalRange native_fixed =
        expanded_fixed16_x(native);
    static_assert(native_fixed.left == 0 && native_fixed.width == 20480 &&
                  native_fixed.right == 20480);
    constexpr PhysicalRect unchanged =
        uniform_safe_rect({10, 20, 110, 220}, native);
    static_assert(unchanged.left == 10 && unchanged.top == 20 &&
                  unchanged.right == 110 && unchanged.bottom == 220);

    constexpr Canvas dual_fhd = make_canvas(3840, 1080);
    static_assert(dual_fhd.active());
    static_assert(dual_fhd.safe_width == 1920);
    static_assert(dual_fhd.safe_left == 960);
    constexpr LogicalRange dual_fhd_logical =
        expanded_logical_canvas(dual_fhd);
    static_assert(dual_fhd_logical.left == -640 &&
                  dual_fhd_logical.right == 1920);
    constexpr PhysicalRect dual_fhd_full =
        uniform_safe_rect({0, 0, 3840, 1080}, dual_fhd);
    static_assert(dual_fhd_full.left == 960 && dual_fhd_full.top == 270 &&
                  dual_fhd_full.right == 2880 &&
                  dual_fhd_full.bottom == 810);

    constexpr PhysicalRect wide_full =
        uniform_safe_rect({0, 0, 5120, 1440}, wide);
    static_assert(wide_full.left == 1280 && wide_full.top == 360 &&
                  wide_full.right == 3840 && wide_full.bottom == 1080);

    // Signed input must retain the original converter's truncation direction.
    assert(logical_width(-1, uw) == -2);
    return 0;
}
