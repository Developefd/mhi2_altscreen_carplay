# Direct Virtual Cockpit video path — Stream 111 to MOST

> Status: **vehicle-proven on the Škoda MU1440 reference unit**
>
> This document describes the downstream path that is already known to work:
> direct CarPlay secondary-screen H.264 -> MPEG-TS -> `/dev/mlb/isoTX2` -> MOST150 ->
> Virtual Cockpit, including the reversible takeover of the native map-video producer.

## 1. Executive summary

The key architectural discovery was that the MHI2 does **not** need to decode the CarPlay secondary
video, render it locally and encode it again for the cluster.

The Virtual Cockpit already contains the decoder used by the stock MHI2 map path.

The shortest proven route is therefore:

```text
iPhone CarPlay navigation
        │
        ▼
Auxiliary / ScreenAlt stream type 111
        │
        ▼
MU1440 AirPlay receiver/interposer
        │
        ├── session/key handling
        ├── Screen AES decrypt
        └── AVCC -> Annex-B H.264
                │
                ▼
         direct-ts-remux
                │
                ▼
       H.264 inside MPEG-TS
                │
                ▼
 strict 64 × 188-byte writes
       = 12032 bytes/write
                │
                ▼
        /dev/mlb/isoTX2
                │
                ▼
       devp-iso-mmx-mib2
                │
                ▼
             MOST150
                │
                ▼
      Virtual Cockpit decoder
                │
                ▼
       visible CarPlay map
```

Apple Maps has been visibly rendered through this complete path on the real reference vehicle.

The remaining project work is therefore **not basic VC video transport feasibility**.

---

## 2. The stock MHI2 map-video path

The stock system already sends a compressed map video stream to the cluster.

The exact MU1440 startup identifies the relevant transport as:

```text
DCIVIDEO: Kombi Map
```

with the transmit endpoint:

```text
/dev/mlb/isoTX2
```

The stock topology is:

```text
native navigation graphics
        │
        ▼
DisplayManager
        │
        ├── NVIDIA H.264 encode
        ├── MPEG transport mux
        └── timing / bitrate / PID handling
                │
                ▼
        /dev/mlb/isoTX2
                │
                ▼
      devp-iso-mmx-mib2
                │
                ▼
             MOST150
                │
                ▼
       Virtual Cockpit decoder
```

This is important because `/dev/mlb/isoTX2` is **not a framebuffer**.

It is downstream of the stock video encoder/mux. The payload expected there is MPEG transport data,
not raw pixels.

---

## 3. Why direct injection works

CarPlay already gives us compressed H.264.

Therefore doing this:

```text
CarPlay H.264
 -> decode on MHI2
 -> render
 -> encode again
 -> MOST
```

would add latency and complexity without being necessary.

Instead we retain the compressed video:

```text
CarPlay H.264
 -> remux only
 -> MPEG-TS
 -> stock MOST transport
 -> VC hardware decoder
```

The first deterministic vehicle experiment used a pre-generated 1010×376 H.264/MPEG-TS test stream.
With the native writer suppressed, a moving test pattern became visible in the exact VC map region.
When the test ended and stock production was restored, the native map returned.

That closed the fundamental transport question before live CarPlay was connected to the sink.

---

## 4. The exact `isoTX2` write contract

A normal single 188-byte MPEG-TS write is **not** accepted by this resource-manager path.

Vehicle probing established the useful application-side message boundary:

```text
1 MPEG-TS packet = 188 bytes

64 packets × 188 bytes
= 12032 bytes

one application write
= exactly 12032 bytes
```

The direct writer therefore buffers complete TS packets until one full 64-packet block is available.

```text
TS packet 0    188 B
TS packet 1    188 B
...
TS packet 63   188 B
             ───────
block        12032 B
                 │
                 ▼
          one write()
                 │
                 ▼
        /dev/mlb/isoTX2
```

A positive short write is treated as a hard error rather than sending a short remainder write. That
preserves the message boundary that produced the working vehicle result.

At end of a finite stream, a partial block is padded with standard MPEG-TS null packets until the
full 12032-byte message size is reached.

---

## 5. MPEG-TS construction used by the direct path

The direct remuxer uses the existing H.264 stream without decoding it.

The tested implementation creates an MPEG-TS output with:

- H.264 video;
- video PID `0x11` on the current path;
- 90 kHz stream time base;
- PAT/PMT generation;
- PMT start PID `4096`;
- periodic PAT;
- PCR cadence configured for the transport;
- startup header resend / initial discontinuity handling;
- null-packet padding where needed for the strict MOST write block.

Conceptually:

```text
Annex-B H.264 access units
          │
          ▼
     MPEG-TS muxer
          │
          ├── PAT
          ├── PMT
          ├── PES/H.264 PID 0x11
          ├── PCR/timestamps
          └── continuity
                 │
                 ▼
          188-byte packets
                 │
                 ▼
          64-packet blocks
```

The current implementation intentionally treats timing, packetization and MOST message framing as a
separate boundary from the CarPlay/UI lifecycle.

---

## 6. Live CarPlay source side

On the reference vehicle the source side has also been demonstrated.

The proven chain includes:

- receiver advertisement including AltScreen/ViewArea capability;
- iPhone `SETUP` for stream type 111;
- receiver listener on the secondary screen connection;
- stock CarPlay session/master-key capture;
- Screen crypto/decryption;
- valid `avcC` codec configuration;
- AVCC -> Annex-B conversion;
- 1010×376 H.264, High Profile Level 3.1 on the proven capture;
- local H.264 consumer/tee path;
- explicit fresh-keyframe request when the downstream consumer attaches;
- fresh IDR production.

The source is then connected to the already-proven direct remux/MOST sink.

```text
Type 111 encrypted screen packets
       │
       ▼
 session/AES handling
       │
       ▼
 AVCC video/config
       │
       ▼
 Annex-B H.264
       │
       ▼
 direct-ts-remux
```

---

# Native map takeover

## 7. The original proof: stop the whole DisplayManager

The first successful direct-TS vehicle test used a deliberately coarse ownership experiment.

Before the custom writer started:

```text
DisplayManager -> SIGSTOP
```

Then:

```text
custom MPEG-TS -> isoTX2 -> MOST -> VC
```

After the bounded test:

```text
DisplayManager -> SIGCONT
```

The observed sequence was:

```text
native map
   │
   ▼
DisplayManager paused
   │
   ▼
moving custom video visible
   │
   ▼
custom writer stops
   │
   ▼
DisplayManager resumes
   │
   ▼
native map returns
```

This proved two things at once:

1. the custom transport works;
2. the stock DisplayManager writer competes with the custom writer for the same Kombi video path.

The control test was equally useful: with the same custom stream and writer parameters but
DisplayManager left active, the custom image did not become visible.

Whole-process `SIGSTOP` was therefore a good experiment but a bad production design.

---

## 8. The vehicle-proven solution: a process-local DisplayManager write gate

The takeover was narrowed to the final data-plane boundary inside DisplayManager.

DisplayManager opens:

```text
/dev/mlb/isoTX2
```

and writes its muxed video through a `writev()` path.

A small process-local interposer tracks the DisplayManager file descriptor associated with
`isoTX2` and changes only what happens to writes on that fd.

### STOCK mode

```text
DisplayManager
   │
   └── writev(fd_isoTX2, stock MPEG-TS)
            │
            ▼
        real writev()
            │
            ▼
      /dev/mlb/isoTX2
            │
            ▼
        native VC map
```

### DIRECT mode

```text
DisplayManager
   │
   └── writev(fd_isoTX2, stock MPEG-TS)
            │
            ▼
       process-local gate
            │
            ├── do NOT forward bytes
            └── report full write length as success

custom direct writer
   │
   └── write(12032-byte MPEG-TS blocks)
            │
            ▼
      /dev/mlb/isoTX2
            │
            ▼
        CarPlay in VC
```

The custom writer is a different process, so its writes are not swallowed by the DisplayManager-local
gate.

This is the crucial distinction: **we do not shut down the MOST channel**. We only stop the native
producer's payload from reaching it.

---

## 9. Why the gate reports success

The gate deliberately behaves like a successful local sink.

It does **not** return an I/O error to DisplayManager.

If DisplayManager were given errors or short writes it could trigger:

- reconnect logic;
- error recovery;
- connection recreation;
- state transitions we do not want.

Instead, while DIRECT is active:

```text
requested bytes = N
actual bytes sent to isoTX2 = 0
return value to DisplayManager = N
```

DisplayManager therefore stays alive and believes its normal pipeline is progressing.

Vehicle testing confirmed the important operational result:

- DisplayManager PID survives;
- native DisplayManager payload is suppressed;
- custom writer remains functional;
- switching back to STOCK restores the native map.

---

## 10. Tracking the device rather than a hard-coded fd

A file descriptor number is not stable.

The gate therefore conceptually follows:

```text
open("/dev/mlb/isoTX2")
        │
        ▼
remember returned fd

writev(remembered fd, ...)
        │
        ├── STOCK  -> real writev
        └── DIRECT -> swallow + success

close(remembered fd)
        │
        ▼
forget fd
```

This also makes the mechanism robust if DisplayManager legitimately closes and reopens the transport
during normal operation.

The current vehicle gate revision was specifically hardened so normal operation does not require
restarting DisplayManager or relying on a permanently fixed descriptor.

---

## 11. Runtime STOCK/DIRECT switch

The current proven DisplayManager gate uses a volatile runtime state.

Current gate marker:

```text
/tmp/mibr-isotx2-gate.direct
```

Conceptually:

```text
marker absent
    -> STOCK
    -> DisplayManager writes pass through

marker active
    -> DIRECT
    -> DisplayManager isoTX2 payload is swallowed
    -> custom writer owns useful payload on the channel
```

This lets takeover happen without restarting:

- DisplayManager;
- `smartphone_integrator`;
- `dio_manager`;
- the MOST transport driver.

That property became important once the basic path was stable: ownership changes should not destroy
the surrounding system merely to exchange one video producer for another.

---

## 12. Entering DIRECT safely

The final Auto-Direct design does not switch the gate merely because CarPlay is connected.

It first requires a usable Stream-111 source.

The high-level sequence is:

```text
CarPlay session active
      │
      ▼
valid Stream-111 source state
      │
      ▼
attach local consumer
      │
      ▼
request fresh keyframe
      │
      ▼
observe fresh video / synchronization proof
      │
      ▼
close native DisplayManager write gate
      │
      ▼
start/maintain direct MPEG-TS output
      │
      ▼
CarPlay visible in VC
```

This avoids unnecessarily blanking/suppressing the stock map before an alternate producer is ready.

The newer GEN2 line adds stricter source/config/IDR generation tracking, but keeps this already-proven
downstream transport and takeover architecture.

---

## 13. Returning to STOCK

Return is deliberately simple at the transport layer:

```text
stop custom direct writer
        │
        ▼
clear/reopen DisplayManager gate
        │
        ▼
stock MPEG-TS reaches isoTX2 again
        │
        ▼
native navigation map returns
```

Vehicle testing proved clean stock recovery without killing DisplayManager.

Auto-Direct also returns to STOCK when its source is no longer trustworthy, including conditions such
as:

- real Stream-111 teardown/state change;
- direct bridge exit;
- explicit disable;
- supervisor/watchdog failure.

The design goal is:

> **custom video exists only while a healthy custom producer owns the path; otherwise the system
> falls back to the stock producer.**

---

## 14. Why NavIgnore is a separate part of the solution

Suppressing DisplayManager payload alone was not sufficient for the complete visible CarPlay result.

With smartphone navigation active, the stock HMI/Kombi arbitration could show:

```text
Mobile Navigation aktiv
```

instead of leaving the normal map video region available for the injected stream.

The standalone NavIgnore behavior keeps the normal Kombi `MAP_VIEW` usable while smartphone
navigation is active.

The combined working concept is therefore:

```text
NavIgnore
   │
   └── keep the cluster's normal map presentation available

DisplayManager write gate
   │
   └── choose which producer supplies MPEG-TS bytes

CarPlay direct writer
   │
   └── provide the alternate H.264/MPEG-TS payload
```

These are distinct responsibilities.

NavIgnore does not replace the transport gate, and the transport gate does not replace the HMI
navigation-arbitration behavior.

---

## 15. Relationship to the Java Direct-VC policy work

A separate Java research line modifies:

- `ChangeDataRate`;
- `ChangeDataRateSequence`.

That work demonstrated how the raw Kombi DSI data-rate request can be preserved while an effective
producer policy is applied, and how the native producer can be driven toward rate 0 without lying
about the vehicle's static capabilities.

It is useful architecture and diagnostic work.

However, the **vehicle-proven current video takeover path** described in this document uses the
DisplayManager-local `isoTX2` write gate.

The project should not conflate the two:

```text
Java Direct-VC policy
    -> higher-level producer/display policy research

DisplayManager writev gate
    -> proven payload-level STOCK/DIRECT arbitration
```

The current project handoff explicitly keeps Java Rate-0 out of the production takeover path unless a
future trace gives a reason to revisit it.

---

## 16. Full proven vehicle chain

The complete path demonstrated across the vehicle work is:

```text
iPhone
  │
  │ CarPlay navigation / Auxiliary Screen
  ▼
Stream type 111
  │
  ▼
MU1440 AirPlay interposer
  │
  ├── session/master-key access
  ├── AES decrypt
  ├── avcC handling
  └── AVCC -> Annex-B
  │
  ▼
1010×376 H.264
  │
  ▼
fresh keyframe on consumer attach
  │
  ▼
direct-ts-remux
  │
  ├── MPEG-TS
  ├── video PID 0x11
  ├── timing/PCR
  └── PAT/PMT
  │
  ▼
64 × 188 B
= 12032 B/write
  │
  ▼
/dev/mlb/isoTX2
  │
  ▼
devp-iso-mmx-mib2
  │
  ▼
MOST150
  │
  ▼
Škoda Virtual Cockpit H.264 decoder
  │
  ▼
visible Apple Maps
```

At the same time:

```text
NavIgnore
   -> keeps normal cluster MAP_VIEW available

DisplayManager process-local gate
   -> STOCK: native TS passes
   -> DIRECT: native TS swallowed

Auto-Direct / GEN2 control
   -> switches ownership only when alternate source state is valid
   -> restores STOCK on failure/state loss
```

---

## 17. What is frozen and what is still open

### Vehicle-proven / frozen baseline

- `/dev/mlb/isoTX2` is the usable Kombi map video injection path;
- the cluster decodes H.264 carried in MPEG-TS;
- MHI-side decode/render/re-encode is unnecessary;
- strict 12032-byte writes work;
- native and custom writers compete if both feed the channel;
- the process-local DisplayManager gate cleanly suppresses native payload;
- the custom writer is unaffected;
- DisplayManager remains alive;
- native map returns in STOCK;
- NavIgnore is needed for the proven full visible smartphone-navigation presentation;
- live CarPlay Type-111 video can reach the real VC through this path.

### Still active research

- correct ScreenAlt UI ownership across provider handovers;
- `suggestUI` / `showUI` lifecycle;
- same-session refresh/reacquire;
- GEN2 consumer synchronization after app changes;
- ViewArea behavior;
- cross-firmware and cross-brand compatibility.

Do not redesign the direct MPEG-TS/MOST path to solve an upstream ownership problem unless vehicle
evidence specifically points back downstream.
