# MHI2 AltScreen CarPlay

**Open research and engineering for bringing CarPlay secondary-screen navigation to the Virtual Cockpit on Volkswagen Group MHI2 platforms.**

> [!WARNING]
> **Research / experimental / use at your own risk.**
>
> This repository is **not a finished SD-card installer** and it is not intended to be a one-click modification package at this stage.
> Current work targets units where the owner already has **SSH shell access**, typically through WLAN or a USB-to-LAN adapter.
> Expect incomplete features, vehicle-specific behavior, reboots and the possibility of an unusable infotainment state if experiments are applied incorrectly.
> Keep recoverable backups and understand the changes before running them on a vehicle.

> [!IMPORTANT]
> The first active target is **Škoda MHI2 / MU1440**. The intended project scope is broader: a reusable solution for compatible **Škoda, SEAT and Volkswagen MHI2** systems. Testers and researchers with other MHI2 variants are welcome.

## Project goals

The long-term objective is a documented, reproducible and open implementation of the relevant CarPlay instrument-cluster / secondary-screen path on MHI2.

Primary goals:

1. **CarPlay AltScreen / Auxiliary Screen map output in the Virtual Cockpit**
2. Understand and control the **Virtual Cockpit video/display path** without destroying stock fallback behavior
3. Understand and implement the receiver-side **CarPlay ScreenAlt/Auxiliary control plane**
4. Understand and, where useful, control **ViewArea** / cluster presentation regions
5. Later investigate **navigation maneuver / arrow integration** and complementary RGI/BAP paths
6. Extend the implementation from the initial Škoda target to compatible **SEAT and Volkswagen** MHI2 variants
7. Preserve enough architecture, build and test knowledge that another developer can continue the work without repeating the reverse engineering from zero

An SD-card based installation/update path may come later. It is deliberately **not the first milestone**.

## Current target

Initial reference platform:

- Brand: Škoda
- Platform: MHI2
- Reference firmware: `MHI2_ER_SKG13_P4526_MU1440`
- Runtime access: SSH
- Current engineering access paths: WLAN or USB-LAN
- Cluster target: MQB Virtual Cockpit / MOST video path

Other MHI2 variants are intentionally in scope, but should not be assumed compatible until their binaries, Java stack, display routing and vehicle behavior are compared.

## Current state

This repository starts while the implementation is still under active research.

What is already established:

- a working MHI2 -> Virtual Cockpit video transport path exists;
- the VC path can be driven through the MHI2 display/MOST stack;
- CarPlay has a dedicated secondary-screen / instrument-cluster architecture using an Auxiliary/ScreenAlt presentation and Type-111 video;
- the relevant CarPlay cluster URLs and their high-level semantics have been mapped;
- normal navigation-app or navigation-ownership changes do **not** appear to require tearing down and recreating Type-111;
- `suggestUI` and `showUI` are distinct parts of the control flow;
- ViewArea changes are an existing-session mechanism, not a new Type-111 stream generation;
- a minimal Java build path for the MU1440 display policy has been established;
- the stock Java display policy can be modified while keeping raw DSI state and a stock fail-safe path intact.

What is **not finished**:

- a complete MU1440 receiver-side ScreenAlt/Auxiliary implementation;
- the complete vehicle-side `suggestUI -> selected UI/showUI` behavior;
- final binding of the incoming CarPlay Type-111 media to the desired VC presentation under all lifecycle transitions;
- full ViewArea control and layout validation;
- broad cross-firmware/cross-brand compatibility;
- a safe end-user installer.

The current project phase should therefore be read as **engineering research with increasingly reproducible PoCs**, not a release.

## Current public developer artifact

The first intentionally published executable component is limited to the already vehicle-proven
**H.264 -> MPEG-TS -> MOST -> Virtual Cockpit** transport layer:

```text
Annex-B H.264
      │
      ▼
direct-ts-remux
      │
      ▼
MPEG-TS
      │
      ▼
64 × 188 = 12032-byte writes
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

Source, reproducible build and developer documentation:

- [direct-ts-remux source](src/native/direct-ts-remux/direct_ts_remux.c)
- [build instructions](docs/build/DIRECT_TS_REMUX_BUILD.md)
- [SSH developer quickstart](docs/testing/DIRECT_TS_REMUX_SSH_QUICKSTART.md)
- [artifact directory](artifacts/mu1440/direct-ts-remux/vehicle-proven-run143/)

The public developer build is intentionally **unstripped** (`-O2 -g`) and accompanied by SHA-256,
build provenance and ELF/symbol reports.

Canonical MU1440 developer binary SHA-256:

`672275fc0e840e604cef68b6bd7b1e9e2059342471997d9bd859e32c34a0f02b`

See the artifact's [reproducibility note](artifacts/mu1440/direct-ts-remux/vehicle-proven-run143/REPRODUCIBILITY.md) for the independent public rebuild.

This artifact does **not** constitute the complete CarPlay AltScreen solution.

### Experimental GEN2 ScreenAlt checkpoint

A second, deliberately conservative developer snapshot is also published for researchers who want to
work above the raw H.264/MOST layer:

- [GEN2 snapshot README](artifacts/mu1440/altscreen111/experimental-gen2-2026-09-25/README.md)
- [GEN2 source](src/native/altscreen111-gen2/)
- [GEN2 SSH workflow](docs/testing/GEN2_EXPERIMENTAL_SSH.md)

Canonical experimental binary SHA-256:

`f3efa9f09972422307f1b9e37e323539036d935f9cc630a1ed7d8f223a2bbae2`

This is **not the newest development binary**. It is an intentionally older vehicle-tested checkpoint
with live VC video and a known lifecycle/recovery limitation. Current development is already further
ahead in automatic start/stop, provider switching, `suggestUI` observation, same-session reacquire
and codec-generation handling.

For the current engineering state, read:

- [Current development status](docs/status/CURRENT_DEVELOPMENT_STATUS.md)
- [Known issues and open work](docs/findings/KNOWN_ISSUES.md)

## Start here

If you want to understand the project without reading the entire repository, use this order:

1. [System overview](docs/architecture/SYSTEM_OVERVIEW.md) — the complete CarPlay -> MHI2 -> Virtual Cockpit architecture and the separation between media and control planes.
2. [Direct VC video path](docs/architecture/DIRECT_VC_VIDEO_PATH.md) — the vehicle-proven Stream-111 -> H.264 -> MPEG-TS -> `isoTX2` -> MOST path, including reversible native-map takeover.
3. [ScreenAlt control plane](docs/architecture/SCREENALT_CONTROL_PLANE.md) — the current handshake, ownership, `suggestUI` / `showUI`, ViewArea and presentation-refresh problem.
4. [Current development status](docs/status/CURRENT_DEVELOPMENT_STATUS.md) — where the implementation has already moved beyond the published binary checkpoint.
5. [Known issues](docs/findings/KNOWN_ISSUES.md) — the exact lifecycle/provider-switch problems still being worked.
6. [Current research status](docs/findings/CURRENT_RESEARCH_STATUS.md) — evidence-ranked table of what is vehicle-proven, binary-confirmed, implemented, open and deferred.

### Current research focus

The main open problem is **not basic Type-111 video transport anymore**.

The current focus is the receiver-side same-session lifecycle:

```text
working Apple Maps in VC
        │
        ▼
route/provider/UI state changes
        │
        ▼
Type-111 often remains the same generation
        │
        ├── suggestUI changes
        ├── navigation ownership changes
        ├── SecondDisplayMode may change
        ├── receiver may need showUI/stopUI handling
        └── existing bitstream may need forceKeyFrame/resync
                │
                ▼
       keep UI ownership + video synchronized
```

That is the current engineering bottleneck and the reason the next vehicle work is heavily instrumented rather than another rewrite of the transport path.

---

# Architecture

There are two separate problems that ultimately have to meet:

1. **CarPlay must create and maintain the secondary navigation presentation.**
2. **MHI2 must place the resulting video into the correct Virtual Cockpit path and arbitrate it against the stock map producer.**

A simplified model is:

```text
┌──────────────────────────── iPhone / CarPlay ────────────────────────────┐
│                                                                          │
│  Navigation app / CarPlay scene                                          │
│             │                                                            │
│             ├── navigation ownership / trip state                        │
│             │                                                            │
│             ├── suggestUI([...cluster URLs...]) ───────────────┐         │
│             │                                                  │         │
│             ├── Auxiliary / ScreenAlt presentation             │         │
│             │          │                                       │         │
│             │          └── H.264 secondary-screen video         │         │
│             │                     │                            │         │
│             └── Type-111 ScreenStream ─────────────────────────┼─────────┼──►
│                                                                │         │
└────────────────────────────────────────────────────────────────┼─────────┘
                                                                 │
                                      CarPlay control plane       │
                                      + Type-111 media            │
                                                                 ▼
┌────────────────────────────── MHI2 head unit ─────────────────────────────┐
│                                                                           │
│  CarPlay receiver / smartphone integration                                │
│            │                                                              │
│            ├── ScreenAlt/Auxiliary capabilities                           │
│            ├── suggestUI handling / URL selection                         │
│            ├── optional showUI/stopUI interaction                         │
│            ├── ViewArea state                                             │
│            │                                                              │
│            └── Type-111 video                                             │
│                    │                                                      │
│                    ▼                                                      │
│            video conversion / transport                                   │
│                    │                                                      │
│                    ▼                                                      │
│            MPEG-TS / MOST / DCIVIDEO                                      │
│                    │                                                      │
│                    ▼                                                      │
│       ┌───────────────────────────────┐                                    │
│       │ Virtual Cockpit video route  │                                    │
│       └───────────────────────────────┘                                    │
│                    ▲                                                      │
│                    │                                                      │
│       stock navigation-map producer                                       │
│                    ▲                                                      │
│                    │                                                      │
│       Java DisplayManagement / DSI policy                                 │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
                          ┌─────────────────────┐
                          │  Virtual Cockpit   │
                          │  map / view area   │
                          └─────────────────────┘
```

The important engineering point is that **media transport and UI/control ownership are related but not identical**. Seeing a valid Type-111 H.264 stream is not, by itself, enough to reproduce the complete stock CarPlay secondary-screen behavior.

## CarPlay secondary-screen model

Current reverse engineering of modern iOS CarPlay frameworks supports the following model.

Normal map-capable navigation advertises the cluster contexts:

```text
maps:/car/instrumentcluster
maps:/car/instrumentcluster/map
```

When a maneuver/instruction card is active, the transient role is added:

```text
maps:/car/instrumentcluster/instructioncard
```

Conceptually:

```text
normal navigation
    │
    └── suggestUI([
          base,
          map
        ])

maneuver card visible
    │
    └── suggestUI([
          instructioncard,
          map,
          base
        ])

maneuver card hidden
    │
    └── suggestUI([
          map,
          base
        ])

trip finish / cancel
    │
    └── suggestUI([])
```

These URLs are best understood as **logical UI contexts**, not separate Type-111 streams.

### The three-way capability intersection

The effective cluster UI candidates are constrained by three sets:

```text
app/provider requested URLs
             ∩
iOS/session cluster URLs
             ∩
receiver-advertised AltScreen URLs
             │
             ▼
       effective URLs
             │
             ▼
        suggestUI
```

This matters for MHI2: advertising the right receiver capability is part of the solution.

### suggestUI is not showUI

A central finding is that `suggestUI` does not simply mean "switch the cluster now".

The current model is:

```text
iPhone -> head unit:
    suggestUI { urls: [candidate contexts] }

receiver evaluates current capability / state

optional receiver decision:

head unit -> iPhone:
    showUI { screen UUID, selected URL }
```

There is no evidence that a failed or ignored `suggestUI` automatically causes iOS to tear down and recreate Type-111.

On the current stock MU1440 path, receiver-side behavior around this control plane is therefore a major remaining implementation target.

## Type-111 lifecycle

A normal navigation-provider change is **not** currently treated as a Type-111 generation boundary.

The intended model is:

```text
existing CarPlay session
      │
      ├── navigation ownership changes
      ├── suggestUI changes
      ├── SecondDisplayMode changes
      ├── ViewArea changes
      └── ordinary trip lifecycle
              │
              ▼
        keep existing Type-111
```

A new Type-111 generation is expected only for an actual transport/session event, for example:

```text
partial AirPlay TEARDOWN
fresh Type-111 SETUP
changed streamConnectionID / session generation
socket/session death
other observed endpoint-topology change
```

This distinction prevents an implementation from adding unnecessary teardown/re-SETUP behavior and chasing the wrong problem.

## ViewArea

ViewArea is another separate mechanism.

Current model:

```text
iPhone requests ViewArea
        │
        ▼
receiver selects/applies layout
        │
        ▼
ViewAreaChanged/update
        │
        ▼
existing ScreenStream / VirtualDisplay continues
```

The project intends to understand the actual VC areas/layouts and eventually expose a controlled implementation instead of treating the cluster as a single fixed rectangle.

## MHI2 -> Virtual Cockpit video path

The video-output side can be viewed separately from CarPlay:

```text
video source
    │
    ▼
H.264 / decoded or remuxed media path
    │
    ▼
MPEG transport stream
    │
    ▼
/dev/mlb / MOST video route
    │
    ▼
DCIVIDEO / cluster transport
    │
    ▼
Virtual Cockpit
```

Existing open work such as OneB1t's VC rendering has been particularly useful in proving that the MHI2/QNX side can feed the MQB cluster route independently of the stock navigation producer.

The difficult part for this project is not merely "can pixels reach the cluster?" but:

```text
Who owns the VC video route right now?
        +
Which CarPlay UI context is active?
        +
Which screen / UUID / ViewArea is selected?
        +
How do we preserve stock behavior when CarPlay is not using it?
```

## Java / DisplayManagement research

The stock MU1440 Java display stack participates in controlling the native map/video producer, and a
separate research line has mapped `ChangeDataRate` / `ChangeDataRateSequence` and demonstrated a
small reproducible Java build island.

That work remains useful for understanding higher-level VC policy, but it is **not the currently
published downstream takeover mechanism**.

The vehicle-proven STOCK/DIRECT video arbitration currently documented by this repository is the
process-local DisplayManager `isoTX2` write gate described in
[DIRECT_VC_VIDEO_PATH.md](docs/architecture/DIRECT_VC_VIDEO_PATH.md).

The Java research remains documented as architecture evidence and may be revisited later; it is not
part of the first public binary artifact.

---

# Access model

For the current development phase you should already have a working shell on your own MHI2 unit.

Typical access:

```text
MHI2
 ├── WLAN -> SSH
 └── USB-LAN adapter -> SSH
```

This repository does **not** currently aim to teach or bundle platform security bypasses. Existing access/recovery methods belong to their respective upstream projects and communities.

An eventual SD-based workflow may wrap mature components later, but it should not hide the underlying changes while the implementation is still experimental.

# Platform strategy

## Phase 1 — Škoda reference implementation

The Škoda MU1440 target is used because it provides a concrete vehicle and firmware against which behavior can be measured.

Goals:

- reproduce stable VC video ownership;
- implement the missing CarPlay ScreenAlt/Auxiliary receiver behavior;
- connect the real Type-111 map stream;
- preserve clean stock recovery;
- understand ViewArea;
- document every state transition and reproducible build.

## Phase 2 — SEAT / Volkswagen comparison

Once the Škoda path is stable:

- compare firmware components and hashes;
- identify identical vs platform-specific Java/native code;
- create compatibility gates instead of blind patching;
- recruit testers with SSH-accessible MHI2 systems;
- move common behavior into reusable code.

# Repository layout

The public repository is deliberately smaller and cleaner than the private exploratory research corpus.

```text
.
├── README.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
├── CONTRIBUTING.md
├── SECURITY.md
│
├── docs/
│   ├── architecture/
│   ├── findings/
│   ├── build/
│   ├── testing/
│   └── firmware-provenance/
│
├── src/
│   ├── native/
│   └── java/
│
├── tools/
├── poc/
├── tests/
└── references/
```

Not every internal experiment belongs here. Public material should be selected because it is one or more of:

- needed to reproduce a finding;
- needed to reproduce a build;
- needed to understand the architecture;
- useful for cross-platform comparison;
- useful for continuing unfinished work.

Large OEM firmware dumps, Apple system binaries and opaque commercial packages are not intended to become a binary mirror.

# Reproducibility and binaries

The project favors **open, inspectable engineering**.

For project-owned code we intend to publish, where practical:

- source;
- build scripts/toolchain pinning;
- hashes;
- build provenance;
- unstripped/debuggable build outputs when redistribution rights permit;
- test procedures and expected evidence.

For third-party/OEM/Apple inputs, the preferred pattern is:

```text
exact product/build identity
+ exact source/provenance
+ SHA-256
+ reproducible extraction instructions
+ symbols/observations that can legally be documented
- no assumption that we can redistribute the original binary
```

# Testers wanted

Useful contributions are not limited to code.

We are particularly interested in testers/researchers who have:

- a Škoda, SEAT or Volkswagen MHI2 unit;
- SSH access to the unit;
- a Virtual Cockpit / compatible digital cluster;
- the ability to capture logs and exact firmware identifiers;
- willingness to test bounded, reversible changes rather than blindly running packages.

Please use **Discussions** first for a new platform/vehicle unless you already have a reproducible bug or concrete research result.

# Issues, Discussions and Pull Requests

Use:

- **Issues** for reproducible bugs, concrete vehicle test reports and actionable research findings;
- **Discussions** for questions, ideas, architecture discussion, new-platform interest and exploratory observations;
- **Pull Requests** for reviewable source, tooling or documentation changes.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening an issue or PR.

# Prior art and credits

This project stands on substantial work by the wider MIB/MQB community.

In particular:

- **Luka and related CarPlay/RGI research** — prior work around CarPlay, route-guidance and cluster integration materially informed the direction of this research. Exact upstream references will be kept in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) / `references/` as they are verified.
- **OneB1t / VcMOSTRenderMqb** — demonstrated a practical MQB QNX/EGL -> DMDT/MOST -> Virtual Cockpit rendering path.
- **MIB Toolbox and the wider M.I.B. community** — provided the open tooling, access and platform knowledge that makes independent MHI2 research practical.
- Researchers who published or preserved observations of other AltScreen implementations have provided useful **prior-art evidence**. Where authorship/licensing is unclear, this project will cite the artifact or public source as research evidence rather than presenting it as project code or implying endorsement.

If you recognize your work and attribution is incomplete or incorrect, please open a Discussion or PR with the original source.

# Independence / trademarks

This is an independent interoperability and reverse-engineering research project.

It is **not affiliated with, sponsored by, or endorsed by** Harman, Apple, Volkswagen AG, Škoda Auto, SEAT/CUPRA, or their affiliates.

Product names and trademarks belong to their respective owners.

# License

Unless a file states otherwise, original project code and documentation are released under:

**GNU General Public License v3.0 or later — SPDX-License-Identifier: GPL-3.0-or-later**

See [LICENSE](LICENSE).

Third-party material retains its original copyright and license. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
