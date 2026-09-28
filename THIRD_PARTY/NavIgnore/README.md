# NavIgnore third-party provenance

The deployable `MIBR-NavIgnore.jar` is the deterministic NavIgnore-only split used by the
vehicle-proven MU1440/AID10 path.

Upstream authority:

- repository: `jilleb/mib2-toolbox`
- commit: `7ec3b7540d48acfebb5d331820b4ee55eff66772`
- upstream path: `Modifications/Lsd/NavActiveIgnore.jar`
- upstream JAR SHA-256: `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`
- upstream repository license: MIT (copy included in this directory)

Deterministic split rule:

- remove exactly:
  `de/vw/mib/asl/internal/mostkombi/streamsink/usecases/ChangeDataRateSequence.class`
- retain all remaining non-`META-INF` payload entries as NavIgnore;
- rebuild a deterministic ZIP/JAR with fixed timestamps and a project manifest;
- no retained class bytecode is recompiled or rewritten by the split.

Published vehicle-tested result:

- path:
  `artifacts/mu1440/navignore/vehicle-tested-2026-09-27/MIBR-NavIgnore.jar`
- SHA-256:
  `b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34`

The corresponding splitter is published as `tools/split_navactiveignore.py`.

This artifact is intentionally separate from the experimental/rate-control Java work. In particular,
the MOST20FPS `ChangeDataRateSequence.class` is not present in this JAR.
