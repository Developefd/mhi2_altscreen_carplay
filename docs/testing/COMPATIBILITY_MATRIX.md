# Compatibility and validation matrix

This table separates **project goal** from **actual evidence**.

## Platform matrix

| Platform | Firmware / baseline | Status | Evidence |
| --- | --- | --- | --- |
| Škoda MHI2 / MU1440 | `MHI2_ER_SKG13_P4526_MU1440` | primary proven target | vehicle-tested |
| other Škoda MHI2 trains | unknown | not assumed compatible | needs testers + exact hashes |
| SEAT/CUPRA MHI2 | planned | unvalidated | help wanted |
| Volkswagen MHI2 | planned | unvalidated | help wanted |
| MHI2Q / Qualcomm | comparator only | not this runtime target | public prior art exists |

Reference MU1440 stock `libairplay.so` SHA-256:

```text
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

A different hash is a **stop condition**, not permission to assume ABI compatibility.

## Functional validation matrix

| Function | Status | Notes |
| --- | --- | --- |
| Annex-B H.264 -> MPEG-TS | PASS | public `direct-ts-remux` |
| strict 64 × 188 / 12032-byte write contract | PASS | vehicle-tested |
| direct write to `/dev/mlb/isoTX2` | PASS | vehicle-tested |
| custom video visible in real VC | PASS | vehicle-tested |
| stock DisplayManager remains alive | PASS | vehicle-tested |
| suppress native producer with writev gate | PASS | vehicle-tested |
| restore STOCK producer | PASS | vehicle-tested |
| Type-111 H.264 receive | PASS | experimental GEN2 snapshot |
| live Apple Maps in VC | PASS | vehicle-tested |
| automatic Direct start/stop | PASS in current development line | newer than public GEN2 binary |
| cable-reconnect recovery | PASS | useful fallback on older GEN2 snapshot |
| manual same-session keyframe recovery | PASS | reproduced twice; fresh source IDR restored moving VC video |
| automatic D2 keyframe recovery | PASS / NEEDS TUNING | kept tested lifecycle/provider transitions usable; current watchdog is intentionally aggressive |
| provider switch without reconnect | PASS in tested 2026-09-27 sequence | Apple Maps / Google Maps / Waze transitions exercised; broader soak testing still needed |
| Google Maps end-to-end | PASS in tested sequence | not yet a broad compatibility claim |
| Waze end-to-end | PASS in tested sequence | rich lower trip-summary UI exposed a SafeArea issue |
| `suggestUI` lifecycle interpretation | STRONG RESEARCH EVIDENCE | exact iOS 27.2 + vehicle tracing |
| navigation composition query controls | PASS / VEHICLE-OBSERVED | ETA/compass/maneuver layout alter real provider composition |
| SafeArea configuration | BUILD-CONFIRMED / NOT YET VEHICLE-VALIDATED | development source exposes configurable geometry |
| dynamic ViewArea | RESEARCH / NOT RELEASED | in-session update mechanism identified; runtime preset switching planned |
| navigation arrows / RGI | FUTURE/SEPARATE | not required for map-video proof |
| SD-card installer | INTENTIONALLY NOT PROVIDED | SSH/developer phase |
| cross-brand portability | OPEN | requires exact target evidence |

## What to include when adding a new target

A useful compatibility report needs:

- brand/model family;
- firmware train + MU;
- stock `libairplay.so` SHA-256;
- DisplayManager binary/config identity if known;
- cluster/AID type;
- SSH transport used;
- result of stock map -> DIRECT -> STOCK sequence;
- project artifact hashes;
- logs with personal/navigation data removed.

Do not post VINs.
