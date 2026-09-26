# Third-party notices, prior art and attribution

This project is independent research. The repository's GPL-3.0-or-later license applies to original project material unless otherwise stated; it does **not** relicense third-party software, firmware, Apple components, OEM binaries or commercial implementations.

## Attribution policy

We try to distinguish three cases.

### 1. Open-source upstream work

Open-source projects that materially informed the implementation should be credited with:

- project/author;
- original repository;
- exact commit/tag where relevant;
- original license;
- what was learned or reused.

If code is actually incorporated, its license obligations must be followed.

### 2. Public technical prior art

Public forum posts, write-ups, videos, logs, diagrams and other published observations may be cited as prior art. Citation does not imply that the original author endorses this project.

### 3. Proprietary / leaked / commercial implementations

Where a proprietary, leaked or commercial binary has been examined for interoperability research:

- it is treated as **evidence/prior art**, not as project source;
- this repository should not claim authorship of it;
- unclear authorship should not be "filled in" by guesswork;
- the original package should not be mirrored merely because it was available elsewhere;
- findings should be re-expressed as independent observations, architecture notes, tests or clean implementation work.

A useful attribution pattern is:

> Comparative research was informed by an independently obtained third-party implementation. The original binary/code is not distributed by this project. Observations are documented only to understand protocol, lifecycle or compatibility behavior.

## Known upstream / prior art

### OneB1t — VcMOSTRenderMqb

Project: `OneB1t/VcMOSTRenderMqb`

Role in this research: important prior art for the MQB QNX/EGL -> DMDT/MOST -> Virtual Cockpit output path.

License observed upstream: The Unlicense / public domain dedication.

### MIB Toolbox

Project: `jilleb/mib2-toolbox` and the wider MIB Toolbox community.

Role in this research: tooling, platform knowledge and open MIB2 engineering practice.

License observed upstream: MIT for the referenced repository.

### M.I.B. / More Incredible Bash community

The wider M.I.B. community has provided substantial open knowledge around MHI2 access, tooling, firmware structure and recovery. Relevant exact references will be added as public documentation is curated.

### Luka — qnx65-armv7-toolchain

Project: `luka-dev/qnx65-armv7-toolchain`

Role in this repository: provides the public QNX 6.5 ARMv7 rebuild environment used by the
independent `direct-ts-remux` validation workflow.

Pinned commit used by this project:

```text
56a66557245af14077678cd28a83ce3a337d9e2d
```

The upstream tree contains multiple third-party/toolchain components with their own licenses. No
single root-project license is assumed here because none was found at the repository root during the
current review. This project therefore treats the toolchain repository as an external build
dependency and does not relicense or vendor it.

### Luka / CarPlay-RGI work

Luka's prior work around CarPlay, route-guidance information and cluster integration has materially influenced the direction of this project.

The exact canonical upstream repository/reference should be recorded here once verified. Until then, do not invent a repository URL or license.

## Apple / CarPlay reverse engineering

Research may document behavior observed in Apple CarPlay frameworks and AirPlay components, including symbols, state transitions, protocol roles, build identities and hashes.

Apple system binaries are not project-owned software and are not relicensed by this repository.

Where exact binaries are required for reproduction, prefer documenting:

- device/build;
- official source;
- SHA-256;
- extraction method/toolchain;
- resulting hash.

## Volkswagen Group / Harman firmware

OEM/Harman binaries and firmware remain the property of their respective rights holders.

This project may document:

- paths;
- component names;
- firmware identities;
- hashes;
- interfaces;
- observed runtime behavior;
- independent patches/tooling where redistribution is lawful.

Large firmware images are not intended to be mirrored in this repository.

## Corrections

If attribution is missing or wrong, please open a Discussion or Pull Request with the original source and, where possible, the applicable license.


## FFmpeg 6.1.5

`direct-ts-remux` statically links a minimal subset of FFmpeg 6.1.5.

Upstream: FFmpeg project

License for the enabled/default library subset: GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later). The public build does not enable FFmpeg's optional GPL or nonfree components.

Pinned source:

- release: `6.1.5`
- source archive: `ffmpeg-6.1.5.tar.gz`
- SHA-256: `b8c8e926b948c14df1264cd0beac1c773df9170ac9cac97bdf1275cd3d385902`

Enabled build surface for this project:

- H.264 demuxer
- H.264 parser
- MPEG-TS muxer
- file protocol
- TCP protocol
- libavformat
- libavcodec
- libavutil

Exact configuration is kept in `tools/build_direct_ts_remux.sh`.

FFmpeg remains copyright its respective contributors and is not relicensed by this project. Before the repository is made public, the downloadable binary publication should be accompanied by the applicable LGPL notice/license and an exact corresponding-source/relink path.
