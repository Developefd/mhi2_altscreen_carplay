# direct-ts-remux — MU1440 developer artifact

This directory contains the first public binary component of the project.

It is deliberately **not an SD-card installer**.

## Purpose

`direct-ts-remux` accepts Annex-B H.264 and remuxes it directly to the MPEG-TS format used by the proven MHI2 -> Virtual Cockpit path.

```text
Annex-B H.264
  -> direct-ts-remux
  -> MPEG-TS
  -> strict 64 × 188 = 12032-byte device writes
  -> /dev/mlb/isoTX2
  -> MOST150
  -> Virtual Cockpit
```

The source is the exact source blob used in the vehicle-proven Run143 downstream path.

## Status

- target: Škoda MHI2 / MU1440 reference unit
- reference firmware: `MHI2_ER_SKG13_P4526_MU1440`
- transport status: **vehicle-proven**
- artifact type: developer / SSH
- stripped: **no**
- build profile: `-O2 -g`

This binary only covers the H.264 -> VC transport side. It does not implement the complete CarPlay ScreenAlt lifecycle, app handover or an end-user installation workflow.

## Files

The committed canonical executable is accompanied by SHA-256, build provenance, ELF headers, sections, symbols, undefined symbols, dynamic dependencies and relocations. See [REPRODUCIBILITY.md](REPRODUCIBILITY.md) for the relationship between the canonical vehicle-lineage binary and the independent public rebuild.

## Input / output

Usage:

```text
direct-ts-remux INPUT OUTPUT FPS MAX_SECONDS WAIT_SECONDS VIDEO_PID
```

For the proven VC transport, OUTPUT is `/dev/mlb/isoTX2`.

Do not write to the device while the stock DisplayManager producer is simultaneously allowed to supply the same channel. See `docs/architecture/DIRECT_VC_VIDEO_PATH.md`.

## Safety

This is a research artifact. Use it only with SSH access, a known compatible target and a tested stock-recovery path. Do not test while driving.
