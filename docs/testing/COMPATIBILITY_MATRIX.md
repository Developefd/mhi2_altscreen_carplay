# Compatibility and validation matrix

This table separates **project goal** from **actual evidence**.

## Platform matrix

| Platform | Firmware / baseline | Status | Evidence |
| --- | --- | --- | --- |
| Škoda MHI2 / MU1440 + AID10-class | `MHI2_ER_SKG13_P4526_MU1440` | primary proven target | vehicle-tested on one 10.x-inch MQB Virtual Cockpit |
| other Škoda MHI2 + AID10-class | unknown | plausible family target, not assumed compatible | needs exact hashes + cluster identity + vehicle test |
| SEAT/CUPRA MHI2 + 10.25-inch Digital Cockpit | planned | likely related 1280x480 family; unvalidated here | help wanted |
| Volkswagen MHI2 + AID10-class | planned | unvalidated | help wanted |
| Volkswagen 12.3-inch AID / other large AID revisions | separate target | **not assumed equivalent to AID10-class** | needs hardware/transport/layout audit + vehicle test |
| Audi B9 MHI2 / AUG22 MU1438 | external compatibility lead | **not supported / not vehicle-tested by this project** | contributor-supplied stock configs show the classic Harman CarPlay process model and support for both MOST-video and second-LVDS/HDMI output configurations; exact cluster/routing/ABI validation required |
| MHI2Q / Qualcomm | comparator only | not this runtime target | public prior art exists |

Reference MU1440 stock `libairplay.so` SHA-256:

```text
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

A different hash is a **stop condition**, not permission to assume ABI compatibility. A matching/known MU firmware is also **not sufficient by itself**: cluster hardware/revision is a separate compatibility dimension.

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
- cluster/AID type, part number and HW/SW identification if available;
- SSH transport used;
- result of stock map -> DIRECT -> STOCK sequence;
- project artifact hashes;
- logs with personal/navigation data removed.

Do not post VINs.


## Cluster-family note

`AID10-class` is a **project shorthand**, not a claim that Volkswagen Group uses one universal official
type name. Public OEM material shows materially different digital-cluster generations/sizes, including
a 12.3-inch 1440x540 Active Info Display and smaller/newer 1280x480 families marketed around
10.25/10.3/11.7 inches depending on brand/model/generation.

For this project, do not infer compatibility from display size alone. Future target admission should gate on:

1. MHI2 firmware/component identity;
2. cluster part number / hardware+software identity;
3. observed display/MOST/DCIVIDEO contract;
4. view geometry / SafeArea behavior;
5. real STOCK -> DIRECT -> STOCK vehicle validation.

TODO: add a read-only `cluster_probe`/preflight report that identifies the cluster family from available
diagnostic/component data. This is intentionally deferred until after the current MU1440/AID10 lifecycle work.


### Public OEM cluster references

These links are background references only; they do **not** establish project compatibility:

- Volkswagen first-generation 12.3-inch AID (1440x540) and newer 11.7-inch AID (1280x480):
  https://www.volkswagen-newsroom.com/en/active-info-display-3950
- Škoda 10.25-inch Virtual Cockpit examples:
  https://www.skoda-storyboard.com/en/press-kits/skoda-scala-press-kit/always-online-thanks-to-the-new-skoda-connect-generation-including-new-infotainment-apps/
- SEAT 10.25-inch Digital Cockpit:
  https://mundoseat.seat.com/mediacenter_netstor/seat-media-center/Img/2018/07/2018-07-31/SEAT-introduces-its-Digital-Cockpit-to-the-Arona-and-Ibiza.pdf
- CUPRA Ateca 10.25-inch Digital Cockpit:
  https://www.cupraofficial.com/content/dam/public/cupra-website/generic/pdf/cupra-ateca-brochure-my25-w38.pdf

The dimensions/marketing names alone must never be used as an installer compatibility gate.
