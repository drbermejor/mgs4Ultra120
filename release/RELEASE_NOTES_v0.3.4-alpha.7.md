# MGS4 Ultra120 v0.3.4-alpha.7

This release expands the optional Native Centered HUD with guarded fixes for
three interface paths that were still incomplete in alpha.6. Ultrawide
rendering, FOV, corrected FPS support, supersampling, the high-resolution
reticle fix, controller option and launcher bypass are otherwise unchanged.

## Native interface fixes

Validated on native Windows at 3440x1440:

- the pause-map plane and live markers retain their intended aspect inside the
  centered pause menu;
- the observed live in-engine Codec feed now matches the aspect of the centered
  Codec frame;
- the verified Mission Briefing composition is fitted into the centered safe
  canvas without shrinking full-output clears.

These corrections operate on exact native producers before D3D11 or D3D12.
They validate caller, resource, allocation, topology and coordinate ranges and
leave unknown states unchanged. Native Centered HUD remains a separate option,
**experimental and disabled by default**.

## Updating

Use the Windows setup EXE/portable ZIP or Linux tarball from this release. The
manual ZIP remains available for a transparent copy-only installation.

An existing Native Centered HUD choice is preserved. The three new sub-options
are enabled when absent, so users who had already opted into the module receive
the fixes automatically:

```ini
CorrectPauseMapAspect=1
CorrectCodecRealtimeAspect=1
CorrectMissionBriefingAspect=1
```

Close the game and disable **Native Centered HUD** in the configurator to return
to the normal HUD without changing any other feature. Advanced users can also
disable one of the three sub-options in `mgs4_native_centered_hud.ini` and
restart the game.

The controller-profile workaround remains opt-in. Enable **Controller profile
fix** only if you need it; leave it disabled for keyboard/mouse or
controller-plus-mouse/gyro input.

## Known limitations

- Only the pinned Steam executable profile is accepted by default.
- The live Codec fix was validated on the identified in-engine feed, not every
  possible Codec content type.
- Mission Briefing normal composition was validated; alternate ticker, help,
  fade and chapter-specific states have not been exhaustively tested.
- The Drebin Shop preview path is guarded but has not been independently
  validated live.
- Original 16:9 title artwork may remain pillarboxed by design.
- Native Windows visuals were validated at 3440x1440. Linux packaging and
  managers are tested, but visual validation under Proton is still pending.
- A complete playthrough across every act and language may reveal additional
  scene-specific exceptions.
- At extreme aspect ratios, a native 16-bit command limit can prevent an
  individual map expansion; that correction then fails closed.

These limitations are why Native Centered HUD remains opt-in.
