# direct-ts-remux SSH quickstart

> Developer test only. This is not an SD-card installer.

This guide covers only the already proven downstream transport component:

```text
Annex-B H.264
  -> direct-ts-remux
  -> MPEG-TS
  -> /dev/mlb/isoTX2
  -> MOST150
  -> Virtual Cockpit
```

It does not set up CarPlay ScreenAlt, disable the native DisplayManager producer, or modify the HMI.

## Preconditions

Before using the binary, confirm:

- SSH access to your own MHI2 unit;
- target is compatible with the QNX 6.5 ARMv7 build;
- exact firmware identity is known;
- a working stock recovery path exists;
- no test is performed while driving.

Reference target used by this project:

```text
MHI2_ER_SKG13_P4526_MU1440
```

## Verify the artifact

On the workstation, verify:

```sh
sha256sum -c direct-ts-remux.sha256
```

After copying the binary to the MHI2, calculate the hash again with a trusted target-side SHA-256
implementation and compare it to the published value.

Do not run an artifact whose hash does not match.

## Stage the binary

Use a temporary developer path first. Example:

```sh
mkdir -p /net/mmx/fs/sda0/mibr-vc-test
chmod 755 /net/mmx/fs/sda0/mibr-vc-test/direct-ts-remux
```

The exact transfer mechanism is intentionally left to the developer's existing SSH/SCP setup.

## Test to a file first

Before touching the MOST device, validate that your H.264 input can be remuxed to a normal file.

Example:

```sh
./direct-ts-remux input.h264 output.ts 20 8 0 0x11
```

Expected properties:

- output file exists and grows;
- output size is MPEG-TS aligned;
- logs do not show a short-write or mux failure.

This separates input/mux problems from the vehicle transport.

## Direct Virtual Cockpit output

The vehicle-proven output endpoint is:

```text
/dev/mlb/isoTX2
```

The proven application-side message boundary is:

```text
64 × 188-byte MPEG-TS packets
= 12032 bytes per device write
```

The remuxer implements this internally.

Example transport invocation:

```sh
./direct-ts-remux input.h264 /dev/mlb/isoTX2 20 8 0 0x11
```

For a live TCP Annex-B source:

```sh
./direct-ts-remux tcp://127.0.0.1:19820 /dev/mlb/isoTX2 30 90 45 0x11
```

## Important: native map ownership

Do **not** treat the command above as a complete takeover procedure.

The stock DisplayManager normally writes its own MPEG-TS to the same Kombi video channel. If both
producers write concurrently, custom video is not a valid test of the direct path.

The project proved a separate process-local DisplayManager `writev()` gate for reversible
STOCK/DIRECT arbitration. That mechanism is documented in:

```text
docs/architecture/DIRECT_VC_VIDEO_PATH.md
```

The public binary being introduced here is intentionally limited to the remux/transport component.

## Recommended bounded test sequence

```text
1. boot normally
2. verify stock VC map
3. verify /dev/mlb/isoTX2 exists
4. verify H.264 input separately
5. establish a reversible STOCK/DIRECT ownership state
6. start one bounded direct-ts-remux run
7. collect stdout/stderr and vehicle observations
8. stop the custom writer
9. restore STOCK ownership
10. verify the native map returns
```

A test should be considered successful only when stock recovery is also confirmed.

## What to report

For a useful test report include:

- vehicle/model/year;
- firmware/MU;
- cluster type;
- `direct-ts-remux` SHA-256;
- public source commit/build ID;
- input resolution/fps/profile;
- command line;
- whether the custom image appeared;
- whether stock map recovery succeeded;
- relevant logs.

Remove VINs, navigation addresses, credentials and private network details before posting.
