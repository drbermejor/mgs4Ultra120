# Technical notes

## World rendering

The patch validates the PE timestamp and image size before enabling hooks. It
intercepts the common engine projection setter at RVA `0x0e3410`. For a
positively identified 16:9 or already-native target-aspect perspective matrix,
it applies the optional FOV multiplier and derives the horizontal scale from
the adjusted vertical scale:

```text
adjusted_m11 = original_m11 / FOVMultiplier
m00 = sign(m00) * abs(adjusted_m11) / (width / height)
```

This is a projection change, not a post-process stretch. At the default
multiplier of 1.000 it is Hor+. Other values change both axes together and do
not alter geometry proportions. Non-perspective and unknown-aspect matrices
are left alone.

Resolution getters at RVAs `0x65c040` and `0x65c030` return the configured
dimensions. The central resolution setter at `0x65f050` replaces incoming
dimensions with those values. This event-driven path replaced an early
diagnostic prototype that rewrote globals every 8 ms.

## D3D12 UI safe area

The proxy intercepts `D3D12CreateDevice` before renderer initialization and
hooks pipeline and direct command-list creation. The UI vertex shader is
matched by its 948-byte DXBC length and 20-byte DXBC header/hash. Only draws
using a pipeline created with that shader receive a centered viewport whose
width is `height * 16 / 9`; the original viewport is restored immediately
after each draw. World pipelines remain full-width.

The selective UI implementation currently covers D3D12 only. A D3D11
equivalent remains future work.

## Proxy and safeguards

`winmm.dll` forwards the original WinMM functions needed by the game and uses
MinHook for engine/D3D12 interception. Expected bytes are checked at relevant
engine hook sites. Unknown executable builds are rejected rather than patched.

The proxy and INI normally survive Steam file updates because `mgs4.exe` is
not modified. Compatibility with a new executable is deliberately not
assumed: its PE identity and hook-site bytes must be added and retested. The
configurators report an unsupported hash, and the DLL fails closed instead of
writing known offsets into an unknown build.

The ultrawide and FPS paths have separate enable flags. When ultrawide is
disabled, the resolution getters, resolution setter and projection function
are not hooked. When the FPS override is disabled, the frame-limit global is
not written. This makes the 120-FPS-only profile independent from all rendering
changes.
