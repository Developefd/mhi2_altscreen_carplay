# MU1440 CarPlay Stream-111 vehicle PoC — 2026-09-29

Status: **vehicle-proven proof of concept**

Reference target:

- Škoda MHI2 / MU1440
- reference firmware: `MHI2_ER_SKG13_P4526_MU1440`
- AID10-class MQB Virtual Cockpit
- CarPlay Auxiliary / ScreenAlt navigation stream, Type 111
- direct H.264 -> MPEG-TS -> `/dev/mlb/isoTX2` -> MOST150 -> Virtual Cockpit path

## Result

The end-to-end CarPlay secondary-navigation path is now visually proven on the reference vehicle with **three independent navigation providers**:

- **Apple Maps**
- **Google Maps**
- **Waze**

All three produced live moving navigation video in the Virtual Cockpit through the same Stream-111/direct-MOST path.

This is the project's clearest vehicle proof so far that the MU1440 reference target can carry the real CarPlay auxiliary navigation presentation into the factory Virtual Cockpit rather than only synthetic test video.

The evidence set is published in [the accompanying media directory](../media/mu1440-stream111-poc-2026-09-29/README.md).

## What the PoC proves

The observed working chain is:

```text
navigation provider
      |
      v
CarPlay Auxiliary / ScreenAlt
      |
      v
Type-111 H.264
      |
      v
GEN2 / direct remux path
      |
      v
MPEG-TS
      |
      v
/dev/mlb/isoTX2
      |
      v
MOST150
      |
      v
Škoda Virtual Cockpit
```

The result is no longer limited to a generated pattern, a static frame, or a single provider. Apple Maps, Google Maps and Waze all render as moving maps in the same vehicle/cluster path.

The factory VC chrome remains outside/above the CarPlay map layer: gear indication, speed, trip information, lane/assist graphics and the upper cluster status region remain vehicle-owned while the map presentation changes with the CarPlay provider.

## Provider-specific presentation

The three providers do not render the auxiliary screen identically.

Observed in this run:

- **Waze** uses a large dark maneuver card and a relatively sparse map presentation.
- **Google Maps** uses a blue maneuver card, its own vehicle marker and a different zoom/layout.
- **Apple Maps** uses smaller visual objects, exposes more map area, shows richer map detail and uses a more perspective-heavy presentation.

These differences are useful evidence that the receiver should not hard-code one provider-specific composition model.

## SafeArea / ViewArea remains open

The current PoC also makes the next layout problem visible: map content can extend beneath parts of the Virtual Cockpit overlay.

The implementation therefore still needs runtime-testable SafeArea/ViewArea handling.

At minimum, two independently selectable geometry profiles are expected to be useful:

1. **full map view** — exclude only the regions actually covered by VC chrome;
2. **reduced / gauges-visible view** — a much narrower central map region when the large gauge overlays are present.

The reduced view is deliberately not assumed to work well until Apple Maps, Google Maps and Waze have each been tested against that constrained geometry.

Runtime switching is important because the effective geometry may need to follow the active VC view rather than remain a single fixed rectangle.

## Motion smoothness / cadence observation

The real CarPlay map video appeared somewhat less smooth than the earlier synthetic pattern/stripe test on the same downstream display path.

That observation does **not** yet identify the cause.

Because the synthetic path was visually smooth, the next useful diagnostic is to capture the incoming/router-side Stream-111 data in parallel with a vehicle run and correlate frame/access-unit arrival timing with the downstream bridge.

The remaining candidates include sender-side cadence, bursty delivery into the receiver, scheduling, frame duplication/drop behavior, or another real-H.264-path effect. The PoC itself does not distinguish them yet.

## What this does not claim

This milestone does **not** mean the implementation is feature-complete or release-ready.

Still open:

- final SafeArea/ViewArea geometry;
- reduced-view layout behavior;
- deterministic provider/app lifecycle handling in every transition;
- final `suggestUI` / `showUI` / ownership policy;
- cadence/jitter characterization;
- broader firmware/cluster compatibility;
- end-user packaging.

The correct description is therefore:

> **MU1440 CarPlay auxiliary-navigation video PoC: successful on the reference Škoda vehicle with Apple Maps, Google Maps and Waze; layout and lifecycle finishing work remains.**

## Public evidence handling

The published media is intentionally conservative:

- cropped to the relevant Virtual Cockpit area;
- metadata removed;
- video audio removed;
- readable locality/street labels blurred only where necessary;
- no generative image editing, inpainting or synthetic scene modification.

The media should be treated as vehicle-test evidence, not as a pixel-perfect reference for final SafeArea geometry.
