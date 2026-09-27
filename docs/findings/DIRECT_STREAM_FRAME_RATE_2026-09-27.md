# Direct Stream-111 frame-rate evidence — 2026-09-27

## Result

A long on-vehicle capture from the GEN2 **pre-TS H.264 mirror** demonstrates that the direct
CarPlay auxiliary-screen path carries a substantially higher native update cadence than the
historical VNC-renderer path used as prior art.

Measured capture:

| Property | Result |
|---|---:|
| H.264 SHA-256 | `568a2ee81e54cca7db02bc25d2733f0a70cfd6370b277745f4f78eb242d9aa67` |
| H.264 bytes | 44,596,753 |
| Resolution | 1010 × 376 |
| ffprobe decodable frames | 16,524 |
| Active wall-clock interval | 593 s |
| Wall-clock average | **27.865 fps** |
| Short-sample median | **32.0 fps** |
| 10 s-window median | **29.09 fps** |
| 10 s-window 10th–90th percentile | 21.33–32.0 fps |
| Capture drops | **0** |
| Frame types in this recording | 1 I + 16,523 P |
| Capture chunks | 16,526 |

The two-frame difference between capture chunks and decoded frames is consistent with framing /
startup material rather than a sustained capture loss; the capture telemetry itself reports
`drops=0`.

## How the wall-clock rate was derived

The H.264 elementary stream itself does not carry useful presentation timestamps for an accurate
wall-clock measurement. `ffprobe` reports a nominal/derived average of 25 fps for the raw
elementary stream, so that value was **not** used as the real vehicle cadence.

The simultaneously captured GEN2 telemetry records cumulative pre-TS capture chunks against
monotonic elapsed seconds.

First active sample:

```text
t+15s  chunks=2
```

Last increasing sample:

```text
t+608s chunks=16526
```

Therefore:

```text
frames ~= 16526 - 2 = 16524
time       608 - 15 = 593 s

16524 / 593 = 27.865 fps
```

The independent `ffprobe -count_frames` result is also **16,524 frames**, providing a useful
cross-check that one capture chunk was effectively one access-unit/frame across this recording.

## 60-second-scale stability

Telemetry-derived windows were:

| Approx. interval | Frames/s |
|---|---:|
| 15–75 s | 27.70 |
| 75–135 s | 28.80 |
| 135–194 s | 29.29 |
| 194–253 s | 30.37 |
| 253–316 s | 29.46 |
| 316–373 s | 30.32 |
| 373–436 s | 23.37 |
| 436–493 s | 25.82 |
| 493–555 s | 26.84 |
| 555–608 s | 26.83 |

The lower later windows are part of real app/navigation activity, not capture drops.

## Architectural significance

The project path is:

```text
CarPlay Stream 111 H.264
  -> decrypt / normalize
  -> MPEG-TS remux
  -> exact 64 × 188-byte MOST writes
  -> /dev/mlb/isoTX2
  -> Virtual Cockpit decoder
```

There is **no intermediate phone-screen VNC capture and no H.264 decode -> framebuffer -> H.264
re-encode stage** in that media path.

That is the main performance advantage, not a synthetic benchmark number: the native auxiliary H.264
stream can retain its original temporal resolution until the final MOST/cluster transport.

For comparison only, the public OneB1t `VcMOSTRenderMqb` README describes its C++ VNC renderer as
around **10 fps** and documents an optional **20 fps** MOST patch for smoother VNC output. Those
numbers describe a different architecture and should not be presented as a controlled benchmark
against this project.

## Keyframe implication

This recording contains only **one I-frame** and 16,523 P-frames. That is valuable evidence for the
later stale-frame diagnosis:

- source and MOST traffic can continue while the VC decoder remains visually stale;
- an ordinary provider/navigation transition may not naturally provide a fresh IDR;
- explicit `forceKeyFrame` requests can therefore be required to restore decoder synchronization.

The later D2 vehicle test deliberately increased IDR refresh behavior and made Apple Maps /
Google Maps / Waze lifecycle transitions usable. The current 1 s watchdog is a stability baseline,
not the final optimized policy.

## Privacy / raw evidence

The raw H.264 capture contains real navigation imagery and is therefore **not published** in this
repository. The project records its exact SHA-256 and the derived statistics instead.

Associated private evidence hashes:

```text
H.264:
568a2ee81e54cca7db02bc25d2733f0a70cfd6370b277745f4f78eb242d9aa67

telemetry:
3668f4ff31cf5e3d48c4b065a0bfda96f6cf3c40e228c43e879dc7fac6d0ab32

capture log:
95c05ecec4886fac849d06877c54033604eeab5a10a4ce2393e8a3514ec63a17
```
