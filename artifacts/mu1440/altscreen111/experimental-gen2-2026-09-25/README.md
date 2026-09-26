# Experimental GEN2 AltScreen snapshot — 2026-09-25

> **Experimental developer snapshot. Not the current development HEAD.**
>
> This binary was selected because it represents a useful, understandable vehicle-tested checkpoint:
> live secondary-screen video worked in the real Virtual Cockpit, while the remaining lifecycle
> failure mode was already known and reproducible.

## Binary

`libaltscreen111.so`

SHA-256:

```text
f3efa9f09972422307f1b9e37e323539036d935f9cc630a1ed7d8f223a2bbae2
```

Properties:

- target: `MHI2_ER_SKG13_P4526_MU1440`
- ABI: QNX 6.5 ARMv7 / EABI5
- build profile: `-O2 -g`
- stripped: **no**
- `.symtab`: present
- `.debug_info`: present
- privacy audit: no personal names, locations, e-mail addresses or private account identifiers found

Public source:

```text
src/native/altscreen111-gen2/
```

## Why this snapshot was chosen

This is intentionally **not** the newest binary.

The snapshot already demonstrated the important middle layer:

```text
stock CarPlay session
       │
       ▼
secondary display / Type 111
       │
       ▼
valid avcC / H.264
       │
       ▼
local Annex-B consumer
       │
       ▼
direct VC transport
       │
       ▼
live navigation in Virtual Cockpit
```

The AVCC compatibility work in this snapshot fixed a real vehicle issue where frames could arrive
before the implementation had accepted a usable codec configuration.

## Vehicle behavior of this checkpoint

Useful behavior demonstrated around this checkpoint:

- live Apple Maps video in the Virtual Cockpit;
- repeated real Type-111 media flow;
- explicit status/reacquire controls;
- provider changes could be tested without rebuilding the binary;
- reconnecting CarPlay provided a reliable recovery path from some stale-state transitions;
- manual same-session reacquire could restore the presentation in relevant cases.

Known limitation:

- after ending/switching navigation, the last valid frame could remain frozen;
- provider switching was not yet deterministic;
- reconnect/reacquire could still be required.

That known limitation is the reason this artifact is labeled **experimental**.

## Development is further ahead

Current development has already moved beyond this binary with work on:

- automatic navigation start/stop;
- improved provider-switch handling;
- stronger separation of stock stream 110 and private stream 111 lifecycle;
- explicit `suggestUI` observation;
- serialized `showUI` / `stopUI` / `forceKeyFrame`;
- transactional VideoConfig handling;
- config + IDR consumer priming;
- stream/codec/consumer generation fencing;
- source/delivery telemetry;
- ScreenAlt URL-role testing.

See:

- [Current development status](../../../../docs/status/CURRENT_DEVELOPMENT_STATUS.md)
- [Known issues](../../../../docs/findings/KNOWN_ISSUES.md)
- [ScreenAlt control plane](../../../../docs/architecture/SCREENALT_CONTROL_PLANE.md)

Those newer development steps are intentionally **not** being promoted as the public binary until
their edge cases are better understood.

## Compatibility gate

The tested target `libairplay.so` SHA-256 is:

```text
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

Do not assume another MHI2 firmware train is compatible because the UI looks similar.

## What this binary is not

It is not:

- an SD-card installer;
- a universal MHI2 patch;
- a finished cross-brand solution;
- proof that every navigation provider transition works;
- permission to skip backups/recovery checks.

It is a developer checkpoint for people who already understand their MHI2 access and recovery path.

## Screenshots

No vehicle/navigation screenshots are published with this snapshot by design.

Navigation screenshots tend to expose street names, home/work locations, recent destinations or
other identifying information. Technical evidence is kept as hashes, logs, source and state-machine
documentation instead.
