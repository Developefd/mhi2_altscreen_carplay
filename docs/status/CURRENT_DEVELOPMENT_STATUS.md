# Current development status

Last updated: **2026-09-26**

> The downloadable experimental GEN2 binary is intentionally **not the newest development build**.
>
> The public binary is a known, understandable vehicle-tested snapshot. Development has already moved
> several steps beyond it, but those newer changes are still being used to isolate lifecycle edge
> cases and are not published as the recommended binary yet.

## Where the project is now

The project has progressed beyond the initial proof-of-concept stages.

The following architecture is established on the Škoda MU1440 reference system:

```text
CarPlay navigation
      │
      ▼
Auxiliary / ScreenAlt
      │
      ▼
Stream type 111
      │
      ▼
H.264
      │
      ▼
direct MPEG-TS remux
      │
      ▼
/dev/mlb/isoTX2
      │
      ▼
MOST150
      │
      ▼
Virtual Cockpit
```

The native map producer can be suppressed and restored without killing the whole DisplayManager
process, and the stock map can be recovered.

## What already works in the current development line

These points are beyond the earliest public PoCs:

- live CarPlay navigation video has been shown in the real Virtual Cockpit;
- the secondary Type-111 stream can be received and converted to the direct VC transport path;
- native-map takeover and STOCK recovery work;
- navigation start/stop can be handled automatically in the current development line;
- the Direct-VC bridge can be started/stopped by supervisor/watchdog logic rather than by a manual
  one-shot command;
- navigation-provider switching is no longer purely theoretical and has worked in vehicle testing;
- at least some provider transitions have completed without requiring a full head-unit reboot;
- a cable reconnect is a known recovery path for the older published snapshot;
- newer development builds include explicit same-session reacquire logic and stronger stream/config/
  IDR generation handling.

This does **not** mean all lifecycle transitions are solved.

## The current engineering problem

The main remaining problem is not the MOST video path.

It is the interaction between:

```text
CarPlay navigation ownership
        +
suggestUI / showUI / stopUI
        +
SecondDisplayMode
        +
existing Type-111 stream lifetime
        +
H.264 config / IDR refresh
        +
receiver-side VC ownership
```

A provider change can occur while the existing Type-111 transport remains alive.

That means this visual symptom:

```text
last navigation frame remains frozen in the VC
```

does **not** automatically mean:

```text
Type 111 has been torn down
```

The current work is classifying exactly which layer failed.

## Important lifecycle finding

Current reverse engineering strongly indicates that normal navigation/provider changes should usually
reuse the existing secondary-screen stream.

The expected model is closer to:

```text
existing Type-111 session
       │
       ├── navigation owner changes
       ├── suggestUI changes
       ├── display mode changes
       ├── ViewArea may change
       └── sender presentation may need reacquire/keyframe
```

rather than:

```text
provider changed
    -> destroy Type 111
    -> build a completely new Type 111 session
```

This distinction is now driving the current test strategy.

## Current recovery / diagnostic direction

The newer development line can explicitly test a bounded same-session recovery sequence:

```text
stopUI
   ↓
showUI(selected cluster URL)
   ↓
forceKeyFrame
```

The goal is to determine whether the sender is merely waiting for presentation/UI ownership to be
reasserted, or whether a deeper stream/consumer generation change occurred.

## Current instrumentation

Recent development work has added instrumentation for:

- `suggestUI` lifecycle;
- PlatformControl commands;
- ScreenAlt URL roles;
- Type-111 stream/session generations;
- VideoConfig arrival;
- SPS/PPS/IDR state;
- consumer priming;
- delivered access-unit counters;
- source heartbeat;
- receiver command completion;
- STOCK/DIRECT ownership state.

The purpose is to stop debugging a frozen VC image as a single symptom and instead identify the exact
layer at which progress stopped.

## Provider switching

Current status should be described as:

**works in some tested transitions, not yet deterministic enough for a stable release.**

Observed useful behavior includes:

- live Apple Maps in the VC;
- switching away from one navigation provider and obtaining live video again after reconnect/reacquire;
- at least one successful Apple Maps -> another provider transition without unplugging;
- improved current development behavior around automatic navigation start/stop and provider changes.

Still under investigation:

- stale last-frame cases after navigation app termination or ownership change;
- whether the receiver must explicitly reselect UI in every relevant transition;
- exact ordering of `suggestUI([])`, ownership release and the next provider's presentation;
- provider-specific differences between Apple Maps, Google Maps and Waze;
- when `forceKeyFrame` is sufficient and when a stronger consumer reset is needed.

## ViewArea / cluster layout

ViewArea work exists, but it is intentionally not considered finished.

The project already has evidence that ViewArea belongs to the existing secondary-display session and
does not require a new Type-111 stream.

Remaining work includes:

- mapping actual VC presentation areas;
- reacting to VC layout/view changes;
- defining safe-area behavior;
- deciding whether receiver-driven or vehicle-state-driven selection is the better abstraction;
- testing across different clusters/brands.

## Why the published binary is older than this document

The project intentionally separates:

```text
current research HEAD
        !=
recommended experimental binary
```

A binary is promoted only when it represents a useful, explainable checkpoint.

The current downloadable GEN2 snapshot was chosen because:

- live secondary-screen video was proven;
- its failure mode is known;
- its source is published;
- it is unstripped and inspectable;
- it provides a useful baseline for other developers.

Newer internal development may be functionally ahead while still being a worse public reference
because several variables are changing at once.

## What help is most useful

The project is especially interested in developers/testers who can contribute one of these:

- another SSH-accessible Škoda/SEAT/VW MHI2 system;
- another digital-cluster/Virtual-Cockpit variant;
- CarPlay/AirPlay receiver knowledge;
- QNX/MOST/DisplayManager expertise;
- repeatable traces of Apple Maps / Google Maps / Waze provider transitions;
- additional reverse engineering of ScreenAlt UI-control semantics;
- safe ViewArea/layout observations.

A useful contribution does not require solving the whole stack. A precise trace that closes one
state transition is valuable.
