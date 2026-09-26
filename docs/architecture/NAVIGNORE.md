# NavIgnore / navigation-arbitration prerequisite

The proven CarPlay-in-VC path has two separate ownership problems:

1. which producer writes video bytes to `/dev/mlb/isoTX2`;
2. whether the Virtual Cockpit remains in the normal map presentation while smartphone navigation is active.

The DisplayManager write gate solves **(1)**.

The NavIgnore behavior solves **(2)**.

## Observed problem

Without the navigation-arbitration patch, smartphone navigation can cause the cluster/HMI to replace
the normal map presentation with the "mobile navigation active" state rather than leaving the map
video surface available for the injected stream.

With the relevant NavIgnore behavior loaded, the normal Kombi `MAP_VIEW` remains usable while
CarPlay navigation is active.

## What the current reference runtime checks

The current Auto-Direct reference script verifies that the running J9 command line contains:

```text
MIBR-NavIgnore.jar
```

before entering DIRECT.

This is a conservative gate based on the vehicle-tested setup.

## Why the JAR is not published here

The historical JAR is derived from/upstream-compatible with patched OEM Java classes. The public
repository does not claim those OEM-derived class files as project-owned GPL source.

We therefore publish:

- the exact functional role;
- the runtime prerequisite;
- the surrounding source/state machine;
- the resulting vehicle behavior;

but do not relicense or mirror OEM-derived bytecode merely for convenience.

## Architectural rule

Do not merge navigation arbitration and video arbitration into one concept.

```text
NavIgnore-style policy
  -> keep logical cluster map presentation available

isoTX2 write gate
  -> suppress native DisplayManager payload only during DIRECT

direct-ts-remux
  -> supply alternate MPEG-TS/H.264 payload
```

This separation made the vehicle tests interpretable and should be preserved by future implementations.
