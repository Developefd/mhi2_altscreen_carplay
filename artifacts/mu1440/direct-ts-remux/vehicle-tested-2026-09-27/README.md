# Vehicle-tested direct-ts-remux — 2026-09-27

This is the direct MPEG-TS bridge used in the 2026-09-27 vehicle run.

```text
SHA-256 b761a8741682e3cbc6e05f5fe1705475c34c4805d1cd1ce09333274a301e735e
```

Observed live telemetry during the successful run showed increasing MOST block/byte counters with:

```text
write_errors=0
short_writes=0
rc=0
```

The strict MU1440 writer contract remains:

```text
64 * 188 = 12032 bytes per MOST write
```

This checkpoint adds the runtime status telemetry used to distinguish a healthy media/MOST path from
a frozen downstream VC presentation.
