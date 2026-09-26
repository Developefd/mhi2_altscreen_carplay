# Public prior art and external research sources

This project deliberately documents the public work that materially informed the research.

The goal is not only attribution. These are the places a new contributor should inspect before
duplicating months of work.

## Core references

### Luka — MHI2Q CarPlay / RGI

https://github.com/luka-dev/mib2q-carplay-rgi

Relevant for:

- CarPlay/RGI architecture;
- HMI/JXE patch lineage;
- native integration patterns;
- route-guidance semantics;
- historical MHI2/MHI2Q issue discussion.

### Luka — QNX 6.5 ARMv7 toolchain

https://github.com/luka-dev/qnx65-armv7-toolchain

Pinned project revision:

```text
56a66557245af14077678cd28a83ce3a337d9e2d
```

Used to make project-owned QNX components publicly rebuildable.

### Luka — JXE reconstruction

https://github.com/luka-dev/jxe2jar

Pinned revision used in related MHI2 Java work:

```text
3bae6e82177c7084a008c42373042e6eebf5653e
```

### OneB1t — VC MOST renderer

https://github.com/OneB1t/VcMOSTRenderMqb

Pinned reference revision:

```text
27ea74abfcd77c16b5a5bdad3a0bfee24f707ab9
```

Important for understanding the MQB Virtual Cockpit rendering/MOST path.

### Lanye — MHI2Q CarPlay / MMI Mirror

https://github.com/Lanye-z/MHI2Q-CarPlay-MMI-Mirror

Useful comparator for MHI2Q native hooks, cluster presentation, watchdog/recovery and Luka-derived
RGI/HMI structure.

### Yuedi — public MHI2Q CarPlay AltScreen

https://github.com/yuedizhibo/MHI2Q-CarPlay-AltScreen

Particularly useful as a public, inspectable AltScreen comparator.

The native `libcarplay_altscreen.so` is useful for studying:

- PlatformControl command classification;
- Type-111 attach/config lifecycle;
- ScreenStream creation;
- `suggestUI`;
- lifecycle/recovery structure.

It is treated as comparator evidence, not as MU1440 source authority.

## Additional useful projects

### chefranov/mhi2-au37x-carplay

Public Harman-MHI2 CarPlay work derived from adjacent Luka concepts. Useful when comparing what must
be rewritten for classic MHI2 rather than MHI2Q.

### JeniCzech92/lsdtool

Useful MIB2 LSD/JXE extraction/rebuild tooling.

### grajen3/mib2-lsd-patching

Historical MIB2 LSD Java/HMI patching lineage.

### adi961/mib2-android-auto-vc

Useful adjacent MHI2/Virtual-Cockpit Java/HMI evidence.

## How third-party/commercial comparators are handled

The project may study lawfully obtained binaries/packages to understand architecture or behavior.

The public repository does **not** mirror proprietary/commercial payloads.

What can be published instead:

- hashes;
- high-level component roles;
- symbol names;
- protocol vocabulary;
- independently derived state-machine findings;
- comparisons against public implementations;
- project-owned clean implementation.

## Why this catalog exists

If a future contributor discovers an old branch, issue, fork or binary that closes one of the
remaining gaps, please add it.

A high-quality source contribution should record:

- repository/project;
- exact commit/tag where possible;
- platform generation;
- what fact it supports;
- whether the evidence is source, binary behavior or secondary commentary.

For more of the intentionally less-polished notes, there is one more file in the repository tree.
