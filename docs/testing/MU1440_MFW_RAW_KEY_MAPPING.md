# MU1440 raw steering-wheel/keypanel mapping

## Goal

Map the physical steering-wheel controls to the **raw** MHI2 keypanel tuples before choosing any
button for runtime ViewArea/SafeArea switching.

Do not infer physical placement from names such as `ROLLER_LEFT` / `ROLLER_RIGHT`. Vehicle
evidence wins.

## Why the raw layer matters

The exact `MHI2_ER_SKG13_P4526_MU1440` Java stack exposes:

```text
DSIKeyPanelListener.updateKey2(...)
  -> raw keyboard ID
  -> raw key ID
  -> raw key state
  -> SystemKeyUtil translation
  -> ASL KeyListener ID
```

The high-level ASL API does not contain a named `KEY_VIEW`, and several raw MFW keys are not
translated into distinct ASL IDs. A high-level listener can therefore lose the identity we need.

The exact stock keypanel handler already logs raw events before that translation:

```text
HK Received: KBD[<kbd>] KEY[<raw-key>] KST[<state>]
```

The first vehicle mapping attempt should use that existing log path rather than patching an OEM Java
class merely to discover button IDs.

## Exact MU1440 raw MFW constants

`KBD_MFW = 4`.

| Raw key | Exact DSI name |
|---:|---|
| 35 | KEY_MFW_MENU |
| 36 | KEY_MFW_ARROW_RIGHT |
| 37 | KEY_MFW_ARROW_LEFT |
| 38 | KEY_MFW_UP |
| 39 | KEY_MFW_DOWN |
| 40 | KEY_MFW_ROLLER_LEFT |
| 41 | KEY_MFW_CANCEL |
| 42 | KEY_MFW_VOLUME_UP |
| 43 | KEY_MFW_VOLUME_DOWN |
| 44 | KEY_MFW_ROLLER_RIGHT |
| 45 | KEY_MFW_AUDIOSOURCE |
| 46 | KEY_MFW_ARROW_A_UP |
| 47 | KEY_MFW_ARROW_A_DOWN |
| 48 | KEY_MFW_ARROW_B_UP |
| 49 | KEY_MFW_ARROW_B_DOWN |
| 50 | KEY_MFW_PTT_ON |
| 51 | KEY_MFW_PTT_CANCEL |
| 52 | KEY_MFW_INFO |
| 53 | KEY_MFW_HOOK |
| 54 | KEY_MFW_HANGUP |
| 55 | KEY_MFW_OFFHOOK |
| 56 | KEY_MFW_LIGHT |
| 57 | KEY_MFW_MUTE |
| 58 | KEY_MFW_JOKER1 |
| 59 | KEY_MFW_JOKER2 |
| 60 | KEY_MFW_INIT |
| 99 | KEY_MFW_SIDEMENULEFT |
| 100 | KEY_MFW_SIDEMENURIGHT |

Raw states:

| State | Name |
|---:|---|
| 0 | KST_RELEASED |
| 1 | KST_PRESSED |
| 2 | KST_DOUBLEPRESSED |
| 3 | KST_LONGPRESSED |
| 4 | KST_LONGPRESSED2 |
| 5 | KST_LONGPRESSED3 |

## Useful exact ASL translations

The exact MU1440 `SystemKeyUtil` translates, among others:

```text
47 -> KEY_MFW_ARROW_A_DOWN
46 -> KEY_MFW_ARROW_A_UP
50 -> KEY_MFW_PTT_ON
57 -> KEY_MFW_MUTE
44 -> KEY_MFW_ROLLER_RIGHT
45 -> KEY_MFW_AUDIOSOURCE
42 -> KEY_MFW_VOLUME_UP
43 -> KEY_MFW_VOLUME_DOWN
48 -> KEY_MFW_ARROW_B_UP
49 -> KEY_MFW_ARROW_B_DOWN
53 -> KEY_MFW_HOOK
```

Raw 35–41 and 58/59 do not receive their own distinct ASL `KeyListener` IDs in the exact
translation switch. This is one reason the physical VC VIEW key must be mapped at the raw layer.

## Mapping procedure

Capture one control at a time:

```text
A  left roller +1
B  left roller -1
C  left roller press (if present)
D  voice/PTT short
E  right roller +1
F  right roller -1
G  right roller press
H  VC VIEW short
I  VC VIEW hold ~2 s
J  remaining right-side controls
```

Repeat VIEW short/hold at least twice and record both the raw tuple and the visible OEM result.

Use the off-unit parser:

```bash
python3 tools/parse_keypanel_trace.py keypanel.log
python3 tools/parse_keypanel_trace.py --summary keypanel.log
python3 tools/parse_keypanel_trace.py --csv keypanel.log > keypanel.csv
```

## Candidate SafeArea UX

If the physical VIEW control reaches the head unit:

```text
short VIEW press -> leave OEM behavior untouched
long VIEW press  -> toggle SafeArea preset A/B
```

A custom long-hold classifier does not require the OEM to already assign a long-press action: a
listener can measure PRESSED -> RELEASED duration and only consume the added long-hold action while
leaving the normal short press alone.

If the VIEW button never appears on the MIB keypanel DSI, it is likely handled entirely on the
cluster side and cannot be used from this head-unit input layer.

## Safety rule

Do **not** install the external VW voice-button patch just to discover key IDs. Its ASL input model is
highly relevant and semantically matches the MU1440, but its bootstrap uses an OEM-class
`-Xbootclasspath/p` shadow. Bootclasspath shadowing remains a separate vehicle-isolation topic.


## Read-only trace-path discovery

The stock handler already emits the raw line, but the exact MU1440 trace reader/sink should not be
guessed from generic QNX or MHI2 knowledge.

Use:

```sh
./keypanel_trace_discovery.sh
```

The script is intentionally read-only. It reports:

- the active `/eso/bin/traceserver` process;
- the traceserver file descriptors;
- whether known QNX trace readers such as `sloginfo`, `traceprinter` or `tracelogger` actually
  exist on this firmware;
- the exact `logging.properties` / `traceConfig.properties` files that are readable on the unit;
- obvious logger/trace device endpoints.

It does **not** mount anything writable, modify logger settings, signal a process or start a trace
utility.

Only after that output is known should a stock-reader command be selected. This preserves the
zero-patch objective and avoids another desktop/QNX capability assumption.
