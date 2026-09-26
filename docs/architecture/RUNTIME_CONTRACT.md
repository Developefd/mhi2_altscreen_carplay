# Runtime contract and state files

This is the interface between the current project-owned components.

A contributor can replace one component without rewriting the others if this contract remains stable.

## ScreenAlt / Type-111 source

Reference configuration:

```text
ALTSCREEN111_PORT=6031
ALTSCREEN111_TEE_PORT=19820
ALTSCREEN111_WIDTH=1010
ALTSCREEN111_HEIGHT=376
ALTSCREEN111_FPS=30
ALTSCREEN111_AUTO_SHOW=0
ALTSCREEN111_URL=maps:/car/instrumentcluster
```

Runtime state:

```text
/tmp/mibr-carplay111.state
/tmp/mibr-carplay111.heartbeat
```

Expected useful state for the current supervisor:

```text
streaming
```

The local Annex-B consumer connects to:

```text
tcp://127.0.0.1:19820
```

## DisplayManager write gate

Control marker:

```text
/tmp/mibr-isotx2-gate.direct
```

Counter reset marker:

```text
/tmp/mibr-isotx2-gate.reset
```

Published stats:

```text
/tmp/mibr-isotx2-gate.stats
```

Log:

```text
/tmp/mibr-isotx2-gate.log
```

Semantics:

```text
marker absent  -> STOCK: tracked DisplayManager writev passes through
marker present -> DIRECT: tracked DisplayManager payload is swallowed while writev reports success
```

## Auto-Direct supervisor

Persistent enable marker in the reference implementation:

```text
/mnt/app/root/mibr-carplay-autodirect.enabled
```

Runtime:

```text
/tmp/mibr-direct-auto-supervisor.pid
/tmp/mibr-direct-auto-watchdog.pid
/tmp/mibr-direct-auto-bridge.pid
/tmp/mibr-direct-auto.heartbeat
/tmp/mibr-direct-auto.state
```

Useful states include:

- `starting`
- `waiting_gate`
- `waiting_navignore`
- `waiting_carplay`
- `waiting_stream111`
- `waiting_displaymanager`
- `direct_starting`
- `direct`
- `stock`
- `stopped`

## MOST sink

Direct video sink:

```text
/dev/mlb/isoTX2
```

Current proven application write unit:

```text
64 MPEG-TS packets × 188 bytes = 12032 bytes
```

The direct remux path uses MPEG-TS video PID:

```text
0x11
```

## Recovery invariant

At every failure boundary the runtime should prefer:

```text
remove DIRECT ownership
 -> allow stock DisplayManager payloads again
```

rather than restarting unrelated CarPlay processes.

The independent watchdog exists specifically so supervisor failure does not leave the flat DIRECT
marker asserted indefinitely.

## Compatibility warning

This contract describes the first MU1440 target.

Paths such as `/dev/mlb/isoTX2` are real target paths, not portable API promises. A different MHI2
train must prove the same contract before reusing it.
