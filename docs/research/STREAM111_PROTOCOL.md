# MU1440 CarPlay ScreenStream / Type-111 protocol contract

This document records the exact low-level contract used by the current MU1440 implementation.

Reference stock binary:

```text
/mnt/app/eso/lib/libairplay.so
size: 703,688 bytes
SHA-256:
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

The target is an older AirPlay 210.81-era receiver. Do not silently substitute crypto/framing rules
from newer CarPlay receiver generations.

## 1. Stock main screen vs project secondary screen

The stock library natively understands the normal CarPlay screen stream.

The project adds support for the instrument-cluster secondary stream:

```text
primary/main screen: stock path
secondary cluster:   Type 111 / ScreenAlt project path
```

Type 111 is removed from the unsupported stock SETUP switch and handled by the project-owned
secondary-screen implementation.

The primary path is intentionally left stock.

## 2. Screen frame envelope

Independent receiver research and the matching AirPlay generation support a ScreenStream frame with
a **128-byte header**.

Useful fields for the current implementation include:

```text
offset 0: little-endian 32-bit body size
offset 4: opcode
body:     opcode-specific payload
```

The two most important opcodes/classes for video are:

- VideoConfig
- VideoFrame

KeepAlive/ignore/housekeeping messages must not be treated as H.264 frames.

## 3. VideoConfig

For this stock generation, VideoConfig body is treated as the AVC decoder configuration record
(`avcC`) itself.

The project extracts:

- SPS;
- PPS;
- NAL length size;
- codec/config generation.

VideoConfig is not handled as a generic encrypted VideoFrame body.

A consumer must not declare itself primed merely because arbitrary media bytes arrived.

## 4. VideoFrame

The stock Screen path uses AES-CTR for the video frame body.

Matching stock call flow:

```text
AirPlayReceiverSessionScreen_ProcessFrames
  -> AES_CTR_Update(...)
  -> ScreenStreamProcessData(...)
```

After decryption the H.264 payload is AVCC/length-prefixed and is converted by the project to Annex-B
for the local consumer.

## 5. Crypto generation: use the stock CarPlay session

The project does not create a second CarPlay authentication/security system.

It observes the established stock session key at the narrow stock security setup seam, then calls the
stock screen-key derivation function:

```text
AirPlay_DeriveAESKeySHA512ForScreen
```

using the stream connection ID.

Conceptually:

```text
stock authenticated CarPlay session
       │
       └── session AES material
                 │
                 + streamConnectionID
                 ▼
          stock screen-key derivation
                 ▼
          Type-111 screen AES key/IV
```

Do not persist or log session key material.

## 6. Exact stock crypto evidence

The pinned MU1440 binary has direct stock call-site evidence for:

```text
AirPlayReceiverSessionScreen_SetSecurityInfo
  -> AES_CTR_Init

AirPlayReceiverSessionScreen_ProcessFrames
  -> AES_CTR_Update

AirPlayReceiverSessionScreen_StopSession
  -> AES_CTR_Final
```

This is why the MU1440 implementation must **not** be rewritten around the ChaCha20-Poly1305 screen
scheme used by newer receiver generations.

## 7. Master-key observation seam

Relevant stock symbols:

```text
AirPlayReceiverSessionSetSecurityInfo
AES_CBCFrame_Init
```

The project narrows the AES observation to calls originating from the proven
`AirPlayReceiverSessionSetSecurityInfo` caller range.

Reference stock function range:

```text
AirPlayReceiverSessionSetSecurityInfo:
0x25b54 ... approximately +0x100 accepted caller window
```

The exact stock call to the AES CBC init PLT occurs inside that range.

This is a compatibility check, not a portable hardcoded ABI promise.

## 8. AirPlayCopyServerInfo ABI

Historical AirPlay/CarPlay source families contain both three- and four-argument variants.

The exact MU1440 binary proves the **four-argument** form:

```c
AirPlayCopyServerInfo(session, properties, macAddress, outErr)
```

ARM evidence shows all four AAPCS argument registers are preserved/used.

The target also has a normal JUMP_SLOT for this symbol, so ordinary ELF preemption is sufficient for
this part of the implementation; it does not require the inline-prologue hook used for locally-bound
session lifecycle functions.

## 9. Dynamic-symbol compatibility

The original target audit checked the project's required dynamic-symbol set against the exact stock
`.dynsym`.

Result at that checkpoint:

```text
required names checked: 29
present:                29
missing:                 0
```

The current implementation also fails closed if mandatory runtime symbols cannot be resolved.

## 10. SETUP responsibilities

For a requested Type-111 stream, the receiver side must maintain a coherent relationship between:

- stream descriptor;
- stream connection ID;
- secondary-display UUID;
- returned data port;
- stock session/security;
- screen generation.

The project prefers to clone/preserve unknown peer descriptor fields and then apply the
Type-111-specific response fields rather than recreating the complete dictionary from assumptions.

## 11. Data port and local tee are different interfaces

Reference defaults:

```text
Type-111 receiver port preference: 6031
local Annex-B tee:                 127.0.0.1:19820
```

The first is part of the CarPlay secondary-screen transport.

The second is project-local IPC between the Type-111 receiver and a local consumer such as
`direct-ts-remux`.

Never expose the local tee beyond loopback without a separate security review.

## 12. Consumer priming rule

Current development treats these as separate generations:

```text
stream/session generation
codec/config generation
consumer generation
```

A newly attached consumer should not be fed stale mid-GOP data and declared healthy.

Preferred sequence:

```text
consumer attaches
 -> request/resynchronize bitstream if needed
 -> accept coherent VideoConfig generation
 -> observe compatible IDR
 -> consumer becomes primed
 -> normal AUs flow
```

This rule is central to provider-switch recovery.

## 13. Generation boundary rule

Ordinary navigation-provider changes are **not** automatically a Type-111 generation boundary.

Reset the transport generation only on concrete evidence such as:

- partial TEARDOWN containing 111;
- fresh SETUP with changed connection/session identity;
- socket/session death;
- actual endpoint topology change.

UI withdrawal, `suggestUI([])`, ownership changes or a frozen last frame are not sufficient by
themselves.

## 14. What to capture in a useful protocol trace

A high-value trace correlates:

- Type-111 SETUP/TEARDOWN;
- connection ID/session generation;
- VideoConfig generation;
- SPS/PPS;
- IDR/non-IDR count;
- source AU count;
- consumer generation/primed state;
- `suggestUI` / `showUI` / `stopUI`;
- `forceKeyFrame`;
- downstream remux block count.

See `docs/testing/VEHICLE_TEST_PROTOCOL.md`.
