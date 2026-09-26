# Runtime reference

This directory contains project-owned **reference runtime logic and diagnostic helpers**.

It is not a packaged installer.

## `auto-direct/`

Vehicle-tested reference state-machine logic for automatic takeover/recovery:

- common runtime helpers;
- write-gate control;
- supervisor;
- independent watchdog;
- start/stop/status;
- enable/disable reference logic;
- ScreenAlt configuration.

Read first:

- `docs/architecture/AUTO_DIRECT_RUNTIME.md`
- `docs/architecture/RUNTIME_CONTRACT.md`

Some scripts reflect the exact first MU1440 environment and are published so the real tested logic is
visible. Treat target paths as compatibility evidence, not universal MHI2 APIs.

## `diagnostics/`

Generic SSH-oriented helpers:

- `gen2_prereq.sh` — compatibility/hash preflight;
- `gen2_status.sh` — current Type-111/control/runtime state;
- `gen2_collect_logs.sh` — collect useful evidence without an SD layout;
- `gen2_reacquire.sh` — bounded same-session reacquire request;
- `gen2_resync.sh` — manual Candidate-D-style resync experiment.

These are intended to make external vehicle reports comparable.

## `isotx2-gate/`

Reference operator controls for the DisplayManager `writev()` interposer.

The project-owned native source lives at:

```text
src/native/isotx2-gate/
```

## `experimental/`

Experimental controls such as ViewArea feature toggles. Presence here does not imply a promoted
feature or a stable cross-platform interface.

## No SD-card requirement

Nothing in this directory makes SD deployment the project interface.

The current contributor workflow assumes existing SSH access over WLAN, USB-LAN or another known
method.
