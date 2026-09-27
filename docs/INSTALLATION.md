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

Different target hashes are stop conditions.

## Prepare

From a workstation checkout:

```sh
tools/prepare_mu1440_sd.sh /path/to/sd/esd
```

Then use the resulting `MHI2AltScreen/` directory on the target.

The deployment contains individually auditable payload files; it deliberately does not publish an
opaque install archive.

### Target-side hash helper

The target needs an executable QNX ARM `sha256sum`. The deployment checks the known project/M.I.B.
SD path first:

```text
/net/mmx/fs/sda0/apps/sbin/sha256sum
```

Alternatively place a compatible helper in `payload/sha256sum` or set `MIBR_SHA256`.
Missing hash support is a hard stop; target/payload verification is not skipped.

## Apply

```sh
./install.sh --check
./install.sh --apply
```

The installer:

1. verifies exact target and payload hashes;
2. refuses to proceed while the experimental DirectVC Java override is installed;
3. stages the tested GEN2 and direct remux runtime internally;
4. prepares the persistent CarPlay Stream-111 preload with a verified stock backup;
5. installs the DisplayManager isoTX2 writev gate with STOCK as the fail-safe/default mode;
6. optionally keeps/installs an exact compatible NavIgnore if supplied separately;
7. sets the initial navigation composition profile to `map-rich`;
8. enables Auto-Direct persistence;
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
