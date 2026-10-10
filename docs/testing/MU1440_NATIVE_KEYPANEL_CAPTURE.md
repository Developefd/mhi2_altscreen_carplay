# MIBR Native Keypanel Capture v1 — MU1440

**Purpose:** Receive the existing Java DebugSPI socket on `127.0.0.1:15001` from an independent QNX ARM executable. Two SSH sessions: capture + live common timeline in shell 1, human notes in shell 2. The binary is **not** part of the video/Parity/PR31 or PR33 runtime.

## Status and constraints

- Java OEM DebugSPI listener observed on `*.15001 LISTEN` on the actual MU1440. Connection and actual HMI event visibility **still require an on-vehicle test**.
- The source is intended for QNX 6.5 ARMv7 and must be built by the project's pinned QNX toolchain. A Linux ARM executable is not interchangeable.
- **Receive only:** it does not send DebugSPI protocol bytes or change filters, J9, logging.properties, LSD, preload, media, CarPlay, `isoTX2`, or autostart. Establishing a TCP connection can activate the Java DebugSPI sink.
- Saves raw `.bin` (16 MiB cap), decoded text (4 MiB cap), and joined NOTE/EVENT chronology. v1.3 uses a flat project-owned SD current-session pointer (`./mibr-keypanel-native-current`), exclusively created without staging/rename. The recorder does not write to `/tmp` or `/dev/shmem`. Session output is inside its own SD directory.
- Default duration 120 seconds; allowed 10..1800 seconds. Stop with Ctrl+C. Test **while parked**, not while driving or operating SSH.
- Treat `KEY_MFW_JOKER1/2` as symbolic names only, NOT proven physical VIEW/Assist assignments. The decoder logs what firmware actually reports.
- If `hk_received_events=0` but `text_frames>0`, check `debugspi-text.log` before assuming no button signal. The MVC/VIEW physical button may be VC-only, or the OEM normal log may not reach the sink.

## Copy to the existing SD directory

Copy these into e.g. `/net/mmx/fs/sda0/esd/carplay-test/keypanel-logger-v2/`, alongside the old scripts, without overwriting them:

- `mibr-keypanel-native` — **actual** ELF QNX ARM binary from successful pinned workflow artifact
- `keypanel_start_native.sh`
- `keypanel_note_native.sh`

For an artifact that has not passed the QNX CI build and file/ELF inspection, **do not install it as a native binary**. Source-only ZIPs must be compiled first.

## Shell 1 — capture + LIVE combined output

```sh
cd /net/mmx/fs/sda0/esd/carplay-test/keypanel-logger-v2
/bin/ksh keypanel_start_native.sh 120
```

The process connects to `127.0.0.1:15001` and creates `./native-<pid>/combined.log`, `debugspi-text.log`, `debugspi-original.bin`, `report.txt`. The shell 1 display continuously shows both `NOTE` and `EVENT` lines from the same file; every 10 s it prints transport counters.

## Shell 2 — annotation

```sh
cd /net/mmx/fs/sda0/esd/carplay-test/keypanel-logger-v2
/bin/ksh keypanel_note_native.sh
```

Examples, submitted **before** the action:

```text
6 kurz
6 lang
6 OEM-Auswahlmenue sichtbar
5 Assistenz kurz
7 Rad rechts +1
8 Rad rechts druecken
```

A NOTE is appended to the same `combined.log` as the firmware EVENT. Each line has a QNX monotonic timestamp, allowing comparisons even when wall date reports `Jan 01` or `NO_DATE`.

## End and share

Ctrl+C in shell 1. Copy its printed session directory from SD to PC, then share `report.txt` and `combined.log` first; keep original `.bin` when parser refinements are needed.

## Build from repository source

The workflow `build-keypanel-native.yml` runs host split-frame tests and builds using the pinned QNX 6.5 / ARMv7 compiler image. Expected ELF ARM EABI executable, softfp-compatible with existing MU1440 build config. Vehicle execution remains a separate validation step.
## Existing pointer after abnormal exit

If startup says `pointer exists` or `active_or_stale_sd_pointer`, inspect `./mibr-keypanel-native-current` and check `pidin ar | grep mibr-keypanel-native`. Only after confirming there is no active capture may you remove this specific project-owned SD marker. Never blindly replace an active capture's pointer. Normal Ctrl+C/timeout cleanup removes only the marker belonging to the current session.

## v1.3 QNX SD-only pointer fix
On the physical MU1440, native pointer staging under /tmp returned `pointer rename: Improper link`, despite a wrapper `mv` probe passing. The recorder no longer writes anything under /tmp. Both SSH sessions must use the same project directory on SD. The recorder publishes `./mibr-keypanel-native-current` with one exclusive direct create and single write; the note console reads that SD file. A previous pointer is not overwritten or automatically deleted; inspect before manual stale cleanup. The v1.3 start wrapper tests SD writability, remounts only the SD when needed, and accepts the QNX canonical path alias. The data remains under `./native-<PID>/`; do not remove incomplete folders from failed prior runs without inspecting them.
