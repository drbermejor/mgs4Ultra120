# MGS4 Ultra120 v0.3.4-alpha.7

This release expands the optional Native Centered HUD with a verified pause-map
fix and guarded experimental adjustments for two interface paths that were
still incomplete in alpha.6. Ultrawide
rendering, FOV, corrected FPS support, supersampling, the high-resolution
reticle fix, controller option and launcher bypass are otherwise unchanged.

## Native interface fixes

Validated on native Windows at 3440x1440:

- the pause-map plane and live markers retain their intended aspect inside the
  centered pause menu;
- the Codec frame and static/prerendered content use the centered canvas, but
  the observed live in-engine 3D feed can remain horizontally compressed;
- guarded Mission Briefing surfaces are adjusted without shrinking full-output
  clears, but control and ticker text can still overflow the safe canvas.

These corrections and partial adjustments operate on exact native producers
before D3D11 or D3D12.
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

- By default, the mod activates only when `mgs4.exe` matches the exact Steam
  version and hash tested for this release. This is independent of the game's
  install folder and also applies under Proton.
- The centered Codec frame is working, but the identified live in-engine 3D
  feed can still look horizontally compressed. Other Codec content types have
  not all been exercised.
- Mission Briefing remains partial: control or ticker text can overflow or be
  clipped. Alternate help, fade and chapter-specific states have not been
  exhaustively tested.
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
