# Škoda MU1440 zoom fix base: real wheel encoder 40 vs KEY38/39

Hi omonob — thanks for publishing [MHI2-Carplay-Maps](https://github.com/omonob/MHI2-Carplay-Maps). We have a reproducible steering-wheel zoom mapping from a Škoda MHI2 MU1440 / 790 AID10.5 and would like to contribute a targeted fix.

## Current behavior and likely gap

Your [MU1440 Škoda profile](https://github.com/omonob/MHI2-Carplay-Maps/tree/main/MU1440_SKODA/790_AID10.5) ships `CarPlayClusterControls.jar` (8,431 bytes; Git blob `289f41b37261fb2263e8dafd06b8b757af86bf22`). Static bytecode inspection of that exact JAR shows `DSIKeyPanelDispatcher.updateKey2()` calls `Mhi2ClusterControlBridge.onKey2()` for KBD4 KEY38/39 (short zoom / held map-provider switching). Its `updateEncoder2()` merely preserves OEM listener forwarding; there is no rotary-to-CarPlay bridge call.

Replacing KEY38/39 by KEY40 **inside the key handler** would not solve this: the physical wheel *press* is a key callback, while physical *rotation* is a signed encoder callback.

## Direct MU1440 test evidence

We used a receive-only DebugSPI capture, with three turns per direction, right wheel first and left wheel second:

| Operator-described action | Five observed signed integer fields | Count |
| --- | --- | ---: |
| Right, first direction (up) | `(4,40,+1,0,1)` | 3 |
| Right, reverse (down) | `(4,40,-1,0,1)` | 3 |
| Left, first direction (up) | `(4,44,+1,0,1)` | 3 |
| Left, reverse (down) | `(4,44,-1,0,1)` | 3 |

All twelve occur as structured inner `0x0110` f902/tag002e records; their five signed integers strongly agree with `updateEncoder2(kbd,encoderId,signedIncrement,extra,validity)`. Exactly six `A wrong DDS Encoder was recognized` warnings follow the six **right** turns; none follow the six left turns. The physical right-wheel **press** was separately associated with KEY2 KBD4/KEY40. This does not independently prove the DebugSPI record tag equals a specific DSI method number, nor which direction is actual visible zoom-in versus zoom-out; polarity still needs a parked native-scale check.

Public sanitized research (no raw personal vehicle logs or republished third-party binaries):
- [12 measured MU1440 rotary records](https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/docs/research/keypanel/MU1440_F902_FIVE_INT_ENCODER_CAPTURE_2026-10-11.md)
- [Read-only offline extractor](https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/tools/decode_keypanel_f902.py)
- [Detailed, specifically Omonob/MHI2 adapter and patch study](https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/docs/research/keypanel/MU1440_OMONOB_MHI2_WHEEL_ENCODER_BINARY_PATCH_OPTIONS_2026-10-11.md)

## Proposed fix base (not yet a compiled or on-car tested patch)

1. Retain KEY38/39 compatibility on vehicles where applicable. Add a separate, optional `updateEncoder2` observation/bridge for **MU1440 KBD4 encoderId40 and signed ±1 detents**. Ignore encoderId44 (left volume), unrelated IDs and the physical KEY40 key *press* in the rotary handler. Do not synthesize KEY38/39 events.
2. Preserve stock DSI listener fan-out, ordering, confirmation/validity flags (including raw 0x80 handling), and exception isolation. Never globally consume/suppress physical encoder callbacks.
3. Route bounded, calibrated relative zoom intents through your *existing* zoom queue/worker (`zoom-in`/`zoom-out`, semantic AirPlay `changeMapZoomLevel`) rather than adding another AirPlay/media producer.
4. Gate the additional CarPlay zoom on the CarPlay **VC map actually visible AND confirmed VC map focus**. A connected/negotiated source111 alone is insufficient. Native navigation, VC menu focus or uncertain state must preserve normal OEM wheel behavior without extra CarPlay zoom. Selectively suppressing native map zoom, if necessary, is a separate semantic MapScale investigation, not a global dispatcher change.
5. Bench-test the twelve tuples, negative increments, KEY2 press vs encoder, left ID44, raw validity 0x81 -> effective1, existing listener count/order, disconnected command worker, stale session generation and rapid direction changes; then validate safely in a parked vehicle with native map, visible CarPlay map and VC menu open.

**Question:** Would you be willing to publish the buildable Java source/build instructions for `CarPlayClusterControls.jar` or point to its exact source/ABI? The published repository currently contains the compiled JAR. We'd prefer a focused, testable source PR over an opaque binary replacement.

This is an evidence-backed fix proposal, not a claim that a replacement vehicle JAR has already been built or validated.
