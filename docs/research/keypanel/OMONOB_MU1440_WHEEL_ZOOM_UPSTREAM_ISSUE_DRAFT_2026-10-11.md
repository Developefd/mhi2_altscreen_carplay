# Upstream issue draft — omonob/MHI2-Carplay-Maps

**Target:** https://github.com/omonob/MHI2-Carplay-Maps/issues/new  
**Proposed title:** Skoda MU1440 zoom fix base: map physical wheel encoder 40 to CarPlay zoom  
**Status:** Ready to submit, **NOT created upstream**: GitHub integration returned HTTP 403 `Resource not accessible by integration` for upstream `create_issue`. The public repository may be read via the connection, but issue creation there is not authorized. Do not misrepresent this file as an upstream issue or PR.

---

Hi @omonob, thanks for publishing [MHI2-Carplay-Maps](https://github.com/omonob/MHI2-Carplay-Maps)! We have a reproducible Škoda MU1440 / AID 790 steering-wheel **rotation** mapping that may help correct the physical-input adapter in the published Free790 package.

### Why this is separate from #9

[Issue #9](https://github.com/omonob/MHI2-Carplay-Maps/issues/9) includes actual map-app zoom behaviour and a Seat report. This one concerns the narrower **Škoda MU1440 physical input not reaching the current key-only CarPlay bridge**. Whether Waze/another iPhone navigation app supports zoom is independent from correct input capture.

### Exact tested target and passive evidence

- Škoda MHI2_ER_SKG13_P4526_MU1440; public profile [MU1440_SKODA/790_AID10.5](https://github.com/omonob/MHI2-Carplay-Maps/tree/main/MU1440_SKODA/790_AID10.5).
- Read-only QNX DebugSPI v1.4 capture: **13,135 complete MLP frames, 4,432,325 bytes, zero resynchronisation/truncation**; twelve structured inner `0x0110` / subtype `f9 02` / tag `0x002e` records containing five signed int32.
- Operator performed three physical detents in each direction, in this order:

| Physical input | Observed five signed ints | Events |
| --- | --- | ---: |
| Right wheel, first direction | `(4,40,+1,0,1)` | 3 |
| Right wheel, opposite direction | `(4,40,-1,0,1)` | 3 |
| Left wheel, first direction | `(4,44,+1,0,1)` | 3 |
| Left wheel, opposite direction | `(4,44,-1,0,1)` | 3 |

Exactly six `AslTargetSystemKeyPanelHandling: A wrong DDS Encoder was recognized` warnings follow the six right-wheel records; none follows the left six. This is strong correspondence to `updateEncoder2(keyboard,encoderId,signedIncrement,extra,validity)`; the record tag `0x002e` itself has not been independently tied to a DSI method ID. The first-direction/opposite-direction names follow the operator test order; actual native map zoom IN/OUT sign needs further calibration.

The wheel **press** separately produces `updateKey2` KBD4/KEY40. A key press and a rotary `encoderId=40` event are **different DSI event types**. No KBD4/KEY38 or KEY39 was observed among the earlier 65 `HK Received` messages.

Evidence (sanitised) and read-only offline decoder:
- https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/docs/research/keypanel/MU1440_F902_FIVE_INT_ENCODER_CAPTURE_2026-10-11.md
- https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/tools/decode_keypanel_f902.py

### Observed Free790 control-path mismatch

We statically examined the published common `CarPlayClusterControls.jar`, **8,431 bytes**, Git blob `289f41b37261fb2263e8dafd06b8b757af86bf22` (identical in published MU1102/MU1367/MU1440 package variants):

1. `DSIKeyPanelDispatcher.updateKey2` calls `Mhi2ClusterControlBridge.onKey2`, which accepts only **KBD4 KEY38/39** to request existing zoom and long-press map-provider cycling.
2. The same replacement dispatcher's `updateEncoder2` simply forwards to existing OEM notification listeners; **it does not call the bridge**.
3. The published MU1440/790 `core.so` already has a session-retaining UDP loopback worker on `127.0.0.1:7032` accepting `zoom-in` and `zoom-out`, and translating these to `changeMapZoomLevel` (0=in, 1=out). Thus the plausible missing link is the **physical Škoda rotary encoder input**, not the native CarPlay zoom-command sender.

Changing `KEY38/39` literals to `KEY40` in `updateKey2` is **not** a rotation fix: it would reinterpret a right-wheel press instead of receiving detents.

### Suggested narrow fix

- Add a version-bound observing `updateEncoder2` callback to the bridge, or register a separate DSI encoder-notification listener (23), once the exact target service instance and J9 ABI are verified.
- Filter `keyboard=4, encoderId=40, extra=0, effectiveValidity=1, signedIncrement != 0`; ignore left `encoderId=44` so OEM volume remains stock.
- Preserve the original listener fan-out, notification-confirmation flag semantics (raw validity `0x80`), ordering, exception isolation, and VC menu scrolling. **Do not globally consume all right-wheel encoder events.**
- Add CarPlay zoom only when **the cluster CarPlay map is actually visible and has map focus**, with correct retained session generation. A merely negotiated source111 connection is not sufficient. Unknown focus => no extra CarPlay command.
- Confirm sign-to-zoom polarity using the native scale before choosing `zoom-in` vs `zoom-out`; keep the control queue bounded and nonblocking.
- Bench-test all twelve observed input tuples, ± and invalid steps, ID44, press vs rotation, raw confirmation 0x81→effective 1, menu vs map context, burst/reversal, reconnect and rollback before deploying a JAR.

Could you share the matching control-JAR source or exact J9-compatible rebuild path for a targeted input fix? Happy to contribute sanitised test fixtures and a minimal patch, while retaining your package's attribution and licence.

**Status:** physical input measured + static binary/JAR audit. **No patched JAR, in-car retrofit, OEM firmware or private vehicle logs attached.** This does not assert a confirmed fix or guaranteed zoom support in every navigation app.

Additional detail: https://github.com/harman-f/mhi2_altscreen_carplay/blob/main/docs/research/keypanel/MU1440_OMONOB_MHI2_WHEEL_ENCODER_BINARY_PATCH_OPTIONS_2026-10-11.md
