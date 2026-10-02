# iOS 27.2 sender-side research authority

This page records the exact Apple-side evidence that currently drives the MHI2 AltScreen state
machine. It intentionally publishes **derived findings and reproducibility metadata**, not Apple
binaries.

## Exact target

- device: `iPhone14,2`
- iOS: `27.2 beta 2`
- build: `24B5089g`
- AirPlaySender source generation: **`1005.8.1.0.0`** (`1005.8.1`)
- IPSW size: `11,347,450,991` bytes
- IPSW SHA-256:
  `17a161307efffbad74ec6f7045af555ea30c61c565724029dbd8a71b8756139d`

Official Apple source used during the research:

```text
https://updates.cdn-apple.com/2026FallSeed/b2cec596-719c-4130-8012-8ceb7f63e568/iPhone14,2_27.2_24B5089g_Restore.ipsw
```

Extraction tooling:

- `blacktop/ipsw` v3.1.724
- Linux x86_64 release archive SHA-256:
  `08ee1747286c2fb26591a568073735c04e9564ca37476c38a7cc69d0d4783dc2`
- `sgan81/apfs-fuse` commit:
  `66b86bd525e8cb90f9012543be89b1f092b75cf3`

Representative remote extraction:

```sh
ipsw --no-color extract \
  --remote \
  --dyld \
  --dyld-arch arm64e \
  --device iPhone14,2 \
  --output work/dsc \
  "https://updates.cdn-apple.com/2026FallSeed/b2cec596-719c-4130-8012-8ceb7f63e568/iPhone14,2_27.2_24B5089g_Restore.ipsw"
```

## Exact extracted binary hashes

| Component | SHA-256 |
| --- | --- |
| AirPlaySender | `ef5daa0e0e0058448e83642a3fecfaa1746877f203b5b04fca95153416406e8d` |
| CarKit | `a45a2e9f4745fefa5e64e036915311d53649c7d95edf886a2c3f571c9f32bd69` |
| CarPlayServices | `568ab7b18afc06010c63d2e60dd10c46dc9a661aa5581cadbd0f122efb1e7c6e` |
| CarPlayDisplayUtils | `1dd8e479f1555be78d91ab038f7a04e006231e49702a33ee8c3f16cf4a47db7b` |
| CarPlaySupport | `8c6b77e48f79063080859183aee46922bd15478fb1e558b467b88c373f13a4a3` |
| CarPlay.framework / CarPlay | `fb5b588570465f9137243853a8445c224bc972529b8b7259e641de3e46606b60` |

These hashes are the compatibility authority for the findings below. Do not silently mix another iOS
build/device slice into the analysis.

## Capability identity: do not confuse four namespaces

The auxiliary-screen path now has a useful four-layer identity model:

```text
CarKit logical capability     AlternateScreen = 0x01
AirPlay SETUP token           "altScreen" in enabledFeatures
screen/display transport      Type 111 / ScreenAlt
global AirPlay feature mask   separate namespace
```

The first mapping is recovered from a public iOS 26.1 CarKit decompile:
`CRCarPlayFeaturesName()` labels bit `0x01` as `AlternateScreen`, and
`CRCarPlayFeaturesAsAirPlayFeatures()` maps that bit to the AirPlay feature string
`"altScreen"`.

The exact iOS 27.2 beta-2 binaries pinned above independently retain the `altScreen`,
`ScreenAlt`, auxiliary-screen and Type-111 lifecycle described below. Until the exact
`24B5089g` CarKit mapper itself is decompiled, treat the numeric `0x01` mapping as strong
cross-version compatibility evidence rather than silently claiming a byte-for-byte 27.2 mapper
proof.

Receiver-side engineering should target the **SETUP negotiation token**: a receiver that supports
this path must accept `"altScreen"` in the successful `enabledFeatures` response. Merely adding
a Type-111 display descriptor to `/info` is topology advertisement, not a substitute for feature
acceptance.

A separate CarKit helper identifies the Ferrite/theme-asset feature family as `0x38`
(GaugeCluster + DataProtocol + PassengerDisplay). AlternateScreen `0x01` is not part of that
family.

The global 64-bit AirPlay `features` mask is also unrelated numerically: public Apple-derived
headers identify root bit 26 as MFi-SAPv1 audio AES and bit 37 as CarPlayControl. Neither is the
canonical AlternateScreen capability bit.

## Exact AirPlay version for this target

The source-generation question is also closed for this exact build. Public IPSW binary diffs show:

```text
iOS 26.5 / 23F77             950.7.1.0.0
iOS 26.6 / 23G5028e         960.4.3.0.0
iOS 27.2 beta 1 / 24B5084k  1005.7.1.0.0
iOS 27.2 beta 2 / 24B5089g  1005.8.1.0.0
```

That matters when interpreting third-party receivers that deliberately advertise
`sourceVersion=950.7.1`: it is a real Apple AirPlay generation from iOS 26.5, not the current
27.2 value and not the AltScreen identifier. Treat it as a receiver compatibility persona unless
an otherwise-identical A/B proves a version-gated behavior.

## 1. Type 111 is a real Auxiliary / ScreenAlt stream

Exact sender-side concepts include:

- `ScreenAlt`
- `AuxiliaryScreen`
- `secondDisplayID`
- `seconddisplay`
- `endpoint_setupAuxiliaryScreenStream()`
- `APEndpointStreamScreenCreate()`

The auxiliary screen is a distinct screen stream, not merely a main-screen mirror alias.

## 2. Provider ownership and stream topology are separate state machines

Normal navigation-provider/trip changes do **not** appear to recreate Type 111.

Current sender-side model:

```text
Type 111 remains established
       │
       ├── navigation ownership changes
       ├── suggestUI candidate set changes
       ├── app/resource/turn state changes
       ├── SecondDisplayMode changes in-place
       └── bitstream may need same-session resynchronization
```

A real Type-111 generation boundary belongs to events such as:

- actual partial AirPlay TEARDOWN;
- fresh Type-111 SETUP;
- changed stream connection/session generation;
- socket/session death;
- proven endpoint topology reconfiguration.

Do not create synthetic teardown/SETUP merely because one navigation app stopped.

## 3. Cluster URL roles

Exact iOS 27.2 code uses:

```text
maps:/car/instrumentcluster
maps:/car/instrumentcluster/map
maps:/car/instrumentcluster/instructioncard
```

Best current classification:

| URL | Role |
| --- | --- |
| base | generic/root instrument-cluster context |
| `/map` | persistent map-specific cluster presentation |
| `/instructioncard` | transient maneuver/turn-card presentation |

These are UI-context roles inside the existing secondary display session, not separate streams.

Normal navigation advertises the base + map candidates. A maneuver-card lifecycle can temporarily add
`/instructioncard`.

The `maps:` scheme is not proof that this mechanism is Apple-Maps-only. CarPlayServices also makes
the generic default cluster URL family available to eligible third-party instrument-cluster scenes.

## 4. suggestUI is a candidate-list transaction

Exact CarKit contains:

- `CARHandleSuggestUI`
- `-[CARSession suggestUI:]`
- the outgoing command literal `suggestUI`

The effective candidate set is derived from:

```text
app/provider requested URLs
        ∩
current iOS/session cluster URLs
        ∩
receiver altScreenSuggestUIURLs
```

The resulting payload contains:

```text
{ "urls": [ effective candidates ] }
```

An empty list is valid:

```text
suggestUI([])
```

It means the sender is withdrawing its current cluster-UI suggestion.

It does **not** mean "destroy Type 111".

## 5. Navigation finish/cancel ordering

Exact iOS 27.2 CarPlaySupport shows that app-driven trip finish/cancel:

1. performs route/trip cleanup;
2. requests navigation-ownership release;
3. sends `suggestUI([])`;
4. continues display/end-trip cleanup.

The ownership-changed callback can arrive asynchronously afterwards.

This is why vehicle traces should not assume one strict timestamp order between URL withdrawal,
ownership callback and mode-state updates.

Pure externally-triggered ownership loss is a distinct path and does not itself prove a direct
`suggestUI([])` edge.

## 6. showUI / stopUI direction is call-path dependent

Do not assign direction from the command name alone.

Exact CarKit has:

- incoming endpoint-notification handling of `showUI` / `stopUI`;
- an outgoing `CARSession showUIForStreamUUID:url:` path.

For debugging, log the concrete caller/callee path and payload, not just the command string.

## 7. forceKeyFrame means resynchronize the existing screen

`forceKeyFrame` reaches the sender-side screen bitstream restart path.

Treat it as a same-session recovery primitive:

```text
existing Type 111
  -> request fresh bitstream/keyframe
```

not as evidence of a new Type-111 connection.

## 8. ViewArea can be requested on an existing screen

Exact CarKit contains:

```text
-[CARSession requestAdjacentViewAreaForScreenID:]
```

and the matching endpoint action ultimately maps to an existing screen/display ID plus ViewArea
index.

This is strong evidence that dynamic cluster geometry should first be explored as an **in-session
presentation transition**, not as "create another Type-111 stream".

## 9. Current implementation consequence

For MU1440:

- preserve Type 111 across ordinary provider changes;
- track stream/session generation independently from codec generation;
- accept fresh VideoConfig on the same Type-111 session;
- prime a new consumer only after compatible config + IDR;
- use `showUI` / `stopUI` / `forceKeyFrame` as bounded same-session recovery experiments;
- rebuild only on an actual transport/session generation boundary.

## 10. Why Apple binaries are absent here

The extracted system binaries are not redistributed.

A contributor can reproduce the analysis from:

- exact official IPSW;
- exact IPSW hash;
- exact extraction toolchain;
- exact extracted binary hashes;
- function names and behavioral findings above.

See also the advanced notes in `.research/DEEP_CUTS.md`.
