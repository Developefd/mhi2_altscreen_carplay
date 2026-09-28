# MU1440 developer deployment

This directory is intentionally **not a consumer/one-click SD-card release**.

It provides a compact developer deployment path for the exact vehicle-proven reference target:

```text
MHI2_ER_SKG13_P4526_MU1440
```

Prerequisite: working SSH/developer access and the ability to recover the unit. The documentation does
not explain how to obtain shell access.

## Prepare the SD-card root on a workstation

From a repository checkout:

```sh
tools/prepare_mu1440_sd.sh /path/to/mounted-sd-root
```

This writes the deployment directly into the SD-card root:

```text
<SD root>/
  install.sh
  uninstall.sh
  status.sh
  PAYLOAD.sha256
  payload/
  runtime/
```

There is deliberately no `esd/` or `MHI2AltScreen/` wrapper directory.

The prepared payload uses the exact 2026-09-27 vehicle-tested GEN2 and direct-ts-remux binaries plus
the current public build-confirmed isoTX2 gate.

The project does **not** redistribute the current NavIgnore JAR because it contains modified
OEM-derived classes. If a developer already has the exact compatible JAR, it may be placed at:

```text
payload/MIBR-NavIgnore.jar
```

Expected SHA-256:

```text
b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34
```

If it is absent, installation continues but reports that the complete vehicle-proven smartphone
navigation presentation used NavIgnore.

## Compatibility helpers

The prepared directory includes project-owned, reproducibly built QNX ARMv7 helpers rather than depending on a pre-existing M.I.B. `apps/sbin` tree:

```text
payload/sha256sum
payload/tee
```

The historical M.I.B. `sed` dependency is intentionally avoided in this deployment; the one required trim operation is implemented with stock `awk`.

The installer uses it by default. An experienced developer may override it with `MIBR_SHA256`, but
only after independently verifying the replacement binary.

## On the unit

Run from the prepared directory:

```sh
./install.sh --check
./install.sh --apply
```

The apply path performs its own hash/firmware gates, creates persistent backups, installs the
Stream-111 hook, installs the DisplayManager writev gate in STOCK-default mode, sets the navigation
profile to `map-rich`, and enables Auto-Direct persistence.

A reboot is required after installation.

After reboot:

```sh
./status.sh
```

The current D2 keyframe policy is deliberately **not permanently enabled by the installer** while the
1 s watchdog is still being tuned. During current development testing it can be enabled explicitly:

```text
/mnt/app/root/altscreen-u2/scripts/gen2_keyframes.sh on
```

## Uninstall

```sh
./uninstall.sh
```

The uninstaller stops/disables Auto-Direct, restores the exact backed-up stock
`smartphone_integrator.json`, restores the exact stock DisplayManager startup policy, and removes
NavIgnore only if this deployment installed it itself.

A reboot is required after restore.

## Scope

This is a developer convenience layer over vehicle-tested lower-level scripts. It does not change
the project's compatibility claims: a different firmware/hash is a stop condition, and matching firmware alone is not enough. The vehicle-proven reference also uses the project's **AID10-class** 10.x-inch MQB Virtual Cockpit family. A 12.3-inch AID or another cluster generation is a separate unvalidated target.


## Raw steering-wheel/keypanel mapping

The prepared developer directory also includes the read-only discovery helper:

```text
runtime/diagnostics/keypanel_trace_discovery.sh
```

It does not patch Java or modify logger state. Its purpose is only to identify the exact stock trace
reader/sink available on the target before collecting the built-in raw key lines:

```text
HK Received: KBD[n] KEY[n] KST[n]
```

Once a trace file has been captured, parse it off-unit with:

```text
python3 tools/parse_keypanel_trace.py --summary keypanel.log
```

Do not guess the physical VIEW button from constant names; map it from vehicle evidence.
