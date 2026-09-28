# Developer installation

The project now publishes a **developer deployment path**, not an end-user SD-card solution.

It intentionally assumes the operator already has shell access and understands MHI2/QNX recovery.
No instructions are provided for obtaining SSH access.

## Reference target only

```text
MHI2_ER_SKG13_P4526_MU1440
libairplay.so SHA-256:
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

Different target hashes are stop conditions. The current vehicle proof is additionally limited to the reference **AID10-class 10.x-inch MQB Virtual Cockpit**. Cluster generation/hardware is a separate compatibility gate; a known MU1440-compatible software baseline does not prove a 12.3-inch AID target.

## Prepare

From a workstation checkout:

```sh
tools/prepare_mu1440_sd.sh /path/to/mounted-sd-root
```

The generated `install.sh`, `status.sh`, `uninstall.sh`, `PAYLOAD.sha256`, `payload/` and `runtime/` paths live directly in the SD-card root. No `esd/` or `MHI2AltScreen/` wrapper directory is created.

The deployment contains individually auditable payload files; it deliberately does not publish an
opaque install archive.

### Target-side compatibility helpers

The prepared developer directory carries its own reproducibly built QNX ARMv7 `payload/sha256sum` and `payload/tee` helpers. This preserves the stable logging/hash behavior previously obtained from M.I.B. without requiring its `apps/sbin` tree.

The current deployment does not require `sed`; the only former runtime use was replaced with stock `awk` because older firmware trains are known not to provide `sed` consistently.

NavIgnore itself is not redistributed because it contains modified OEM-derived classes. The installer fails closed unless the exact required JAR is already installed or is placed at `payload/MIBR-NavIgnore.jar`.

`MIBR_SHA256` remains an expert override for a separately verified compatible helper. Missing or
non-executable hash support is a hard stop; target/payload verification is never skipped.

## Apply

```sh
./install.sh --check
./install.sh --apply
```

The installer:

1. verifies exact target and payload hashes;
2. refuses to proceed while the experimental DirectVC Java override is installed;
3. stages the tested GEN2 and direct remux runtime internally;
4. preflights and installs the DisplayManager isoTX2 writev gate with STOCK as the fail-safe/default mode;
5. prepares the persistent CarPlay Stream-111 preload with a verified stock backup;
6. requires the exact vehicle-proven NavIgnore (`b065bab0…`) either already installed or supplied separately, and installs it when supplied;
7. sets the initial navigation composition profile to `map-rich`;
8. prepares Auto-Direct persistence **without starting a pre-reboot takeover**;
9. requires a reboot rather than hot-restarting the CarPlay process stack.

## Keyframe policy

The vehicle-proven D2 keyframe policy is available, but the current 1 s watchdog is intentionally
still under tuning. The installer therefore does not silently make that policy permanent.

## SafeArea

The public development source/helper contains configurable SafeArea work, but the stable installer
continues to use the exact 2026-09-27 vehicle-tested GEN2 binary until the SafeArea-capable candidate
has completed its vehicle validation.

## Restore

```sh
./uninstall.sh
```

Restore paths are hash-gated and preserve evidence/runtime files for diagnosis.

See `deployment/mu1440/README.md` for the compact operator view.


## Compatibility helpers and hidden dependencies

The prepared developer directory contains its own public, reproducibly built QNX ARMv7
`sha256sum` and `tee` compatibility helpers. Installation no longer depends on a pre-existing M.I.B.
`/apps/sbin` SD-card tree. The installer also explicitly remounts the active SD/USB deployment medium writable and performs a write test before relying on persistent media logging.

The preflight also verifies the exact command set required by the currently published scripts
before any persistent mutation. This is intentional: common Linux command names/options are not
assumed to exist on QNX.

The top-level preflight also rejects the experimental DirectVC Java override, legacy Most20FPS
bootclasspath patches, and the older combined `NavActiveIgnore.jar` before persistent changes are
made.
