# Most20FPS third-party provenance

The deployable `MIBR-Most20FPS.jar` is the deterministic one-class split used by the
vehicle-validated MU1440/AID10 reference path.

Upstream authority:

- repository: `jilleb/mib2-toolbox`
- commit: `7ec3b7540d48acfebb5d331820b4ee55eff66772`
- upstream path: `Modifications/Lsd/NavActiveIgnore.jar`
- upstream JAR SHA-256: `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`
- upstream repository license: MIT (copy included in this directory)

Deterministic split rule:

- retain exactly `de/vw/mib/asl/internal/mostkombi/streamsink/usecases/ChangeDataRateSequence.class`;
- remove the remaining NavIgnore/navigation-arbitration classes;
- rebuild a deterministic ZIP/JAR with fixed timestamps and a project manifest;
- retained class bytecode is not recompiled or rewritten by the split.

Published vehicle-tested result:

- path: `artifacts/mu1440/most20fps/vehicle-tested-2026-09-29/MIBR-Most20FPS.jar`
- SHA-256: `dbd45609fe4ba69948d39e9e649b224484f680f6aa7934b68c261a4d360ea5bb`

The corresponding splitter is published as `tools/split_navactiveignore.py`.

Vehicle validation on the exact MU1440 reference target captured accepted stock DisplayManager
MPEG-TS immediately before `/dev/mlb/isoTX2`:

- stock: 1010x376 H.264 Baseline, 10 fps, about 100 ms frame spacing;
- Most20: 1010x376 H.264 Baseline, 20 fps, about 50 ms frame spacing;
- four-second capture size and 12,032-byte write count both doubled;
- the five-frame GOP remained, changing native I/IDR cadence from about 0.5 s to about 0.25 s.

This artifact is intentionally separate from `MIBR-NavIgnore.jar`; the split pair owns disjoint
classes and can be loaded together.
