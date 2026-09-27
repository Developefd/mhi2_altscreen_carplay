# Vehicle-tested GEN2 checkpoint — 2026-09-27

This directory contains the exact GEN2 binary checkpoint used for the 2026-09-27 MU1440 vehicle
tests.

Reference target:

```text
MHI2_ER_SKG13_P4526_MU1440
```

## Exact payloads

```text
libaltscreen111.so
SHA-256 094e3f1abfbf949f8c11b27e048e8213e5fc71178e62efd3c56deed8ee3bf8d8

```

## Vehicle result

This checkpoint produced moving CarPlay navigation video in the real Virtual Cockpit.

A stale-frame state was reproducible while Stream 111 stayed alive, H.264 access units continued,
the direct remux continued writing successful MOST blocks and no write failure was present.

Two manual same-session `forceKeyFrame` requests each produced a newer source IDR and immediately
restored moving video.

The D2 automatic policy then kept the tested Apple Maps / Google Maps / Waze route start/stop and
provider transitions usable without another manual recovery.

## Important limitation

The D2 policy in this checkpoint uses a 1 s "no newer IDR" watchdog. It is intentionally conservative:
once enabled, it can force roughly periodic IDRs even when no lifecycle event is occurring. This is
functionally useful for the current vehicle proof but is not the final bandwidth/control policy.

The newer public development source includes SafeArea configuration work that was **not yet vehicle
validated in this binary**.

See:

- `docs/findings/KEYFRAME_RECOVERY_2026-09-27.md`
- `docs/testing/NAV_COMPOSITION_MATRIX_2026-09-27.md`
- `docs/architecture/NAVIGATION_COMPOSITION_AND_SAFEAREA.md`
