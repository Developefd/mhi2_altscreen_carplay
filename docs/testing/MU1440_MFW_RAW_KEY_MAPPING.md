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

The stock keypanel handler path is `AslTargetSystemKeyPanelHandling.processKeyEvent(...)`. In the
decompiled MHI2 family implementation, it logs the raw tuple **before** forwarding the event through
`SystemKeyUtil`:

```text
HK Received: KBD[<kbd>] KEY[<raw-key>] KST[<state>]
```

The first vehicle mapping attempt should use that existing log path rather than patching an OEM Java
class merely to discover button IDs. If the exact MU1440 trace sink cannot expose this line live,
`AslTargetSystemKeyPanelHandling` is the precise fallback observation seam to revisit; even then the
preferred change is additive/passive logging, not key consumption or remapping.

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
| 6 | KST_APPROACHED |
| 7 | KST_ABANDONED |
| 8 | KST_MOVED |

The three distinct OEM long-press stages are intentionally preserved. For mapping work, hold a
candidate control long enough to see whether the hardware/firmware emits only `LONGPRESSED` or also
`LONGPRESSED2` / `LONGPRESSED3`; this may expose additional stock semantics without inventing a
project-side timer.

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

## 2026-10-11 vehicle correction — do not use speculative left/right names as physical IDs

The 300-second passive DebugSPI capture was decoded offline: 18,571 MLP frames, 524 inner OEM text messages, 65 `HK Received` events. The physical operator annotations strongly associate **right-wheel PRESS with `KBD=4 KEY=40`** (symbolically `KEY_MFW_ROLLER_LEFT`) and **left-wheel PRESS with `KBD=4 KEY=44`** (symbolically `KEY_MFW_ROLLER_RIGHT`). The symbolic LEFT/RIGHT terms demonstrably must not be treated as physical mounting positions. Reconfirm a single isolated button if side or mode is uncertain.

The physical right roller's *rotation* does not generate an `HK Received` line in this capture: its manually annotated +/- periods instead coincide with five/eight `wrong DDS Encoder` warnings. The separate 21-second vehicle video shows the right roller changing the **OEM instrument-cluster navigation map scale and on-screen scale/OK overlay**. This is an encoder-path issue, not proof that the wheel is invisible to the car. Exact `updateEncoder2(keyboard, encoderId, increment, extra, validity)` numeric tuple is still unknown.

Stock `AslTargetSystemKeyPanelHandling` routes `updateKey2` and `updateEncoder2` separately; its normal-level `HK Received` log is KEY ONLY. Its `processEncoderEvent` accepts IDs 16, 17 or 44 and warns for other IDs. The Omonob Free790 MU1440 patch in `CarPlayClusterControls.jar` intercepts `DSIKeyPanelDispatcher.updateKey2` solely for `KBD4/KEY38/39` -> zoom and leaves `updateEncoder2` to OEM delegates, so it cannot zoom from this observed rotary motion by itself. See [full wheel/DSI/Omonob audit](../research/keypanel/MU1440_WHEEL_DSI_AND_OMONOB_2026-10-11.md) and [sanitized capture matrix](../research/keypanel/MU1440_CAPTURE_SANITIZED_MATRIX.tsv). No raw vehicle files/video are published.

## Physical Octavia 5E Virtual Cockpit controls

The 2019 Octavia 5E owner-manual behavior and matching VC steering-wheel layout give us the
**physical function layer**, which must remain separate from the raw DSI-ID layer.

| Physical control | OEM function | Raw-ID status on MU1440 |
| --- | --- | --- |
| Left voice button | switch voice control on/off | strong exact-target path: raw `50 KEY_MFW_PTT_ON` -> ASL key 15; confirm tuple on-car |
| Left volume roller, rotate | volume up/down | no `HK` in annotated session; exact encoder/volume path not captured; 42/43 are OEM key constants, not proof of physical detent mapping |
| Left volume roller, press | sound/mute on/off | operator-tagged left centre: **observed `KBD4/KEY44`**, despite symbolic `ROLLER_RIGHT`; isolated recheck recommended |
| Left previous/next controls | previous / next track or station | OEM function confirmed; exact choice among raw arrow A/B keys remains vehicle-trace open |
| Right assistance button, upper-left | open assistance-systems menu | physical function confirmed; raw key still open |
| **Right VIEW button, lower-left** | **short: change VC display version; hold: open the pre-selection menu** | **raw key still open; do not infer JOKER1/JOKER2** |
| Right scroll wheel, rotate | select data / set values / move in menus; VC native map scale visibly changes in video | encoder callback reached stock ASL warning path in tagged periods; exact numeric `updateEncoder2` tuple open; separate from wheel press |
| Right scroll wheel, press | show/confirm selected item; with map turn+press enables automatic map-scale change | **observed `KBD4/KEY40`**, including `1,3,4,5,0` long press; symbolic name `ROLLER_LEFT` is not physical side |
| Right menu/back control | display main menu / return to previous level; context can expose telephone menu | operator-tagged right Back: **observed `KBD4/KEY41`** incl. long-press stages; distinguish from separate VIEW control |

For the VIEW button specifically, the OEM behavior invalidates the earlier idea of using a long hold
as a project-only SafeArea toggle. **Long VIEW is already occupied by the stock preset-selection
menu** (Auto / Classic / View 1 / View 2 / View 3). Both short and long VIEW behavior must therefore
remain untouched.

The preferred project behavior is passive/synchronous:

```text
physical VIEW event
  -> stock VC behavior continues unchanged
  -> project observes raw tuple and resulting VC layout/state
  -> project selects ViewArea 0 or 1
  -> CarPlay relayout follows through updateViewArea
```

Only if a later vehicle trace proves a genuinely unused gesture should an additional project-only
gesture be considered.

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

For the preferred live test use two SSH sessions.

Session 1:

```sh
ksh runtime/diagnostics/keypanel_capture.sh --auto
```

Session 2:

```sh
ksh runtime/diagnostics/keypanel_note.sh
```

Enter a label before each action. Both the label and the next stock key events are written into one
ordered log. If `--auto` cannot use a proven stock reader, run the discovery helper and pipe the
confirmed reader into `keypanel_capture.sh --stdin`.

Use the off-unit parser:

```bash
python3 tools/parse_keypanel_trace.py --gestures keypanel.log
python3 tools/parse_keypanel_trace.py --summary keypanel.log
python3 tools/parse_keypanel_trace.py --csv keypanel.log > keypanel.csv
```

## Target ViewArea UX

The first target is two predeclared pairs:

```text
ViewArea 0 / SafeArea 0 = MAP_FULL
ViewArea 1 / SafeArea 1 = GAUGE_REDUCED
```

Do **not** remap VIEW short or long press. Instead observe the stock event/state and mirror the
resulting VC layout into `updateViewArea(0|1)`.

If the VIEW button never appears on the MIB keypanel DSI, the project must derive the resulting VC
layout from another head-unit-visible state rather than intercepting the button itself.

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


## Evidence sources for the physical-function layer

The physical-function descriptions above are intentionally not used as proof of raw key IDs.

- Škoda Octavia 5E 07/2019 owner's manual, digital-instrument-cluster operation:
  VIEW short changes display version; VIEW hold opens pre-selection; right roller selects/confirms
  and controls map scale.
- The same owner's manual, multifunction-steering-wheel operation:
  voice, volume/mute, next/previous, assistance-menu and display-menu functions.
- A period Octavia RS245 owner report independently shows the VC-equipped right-side physical layout
  as assistance button above VIEW, followed by the option scroll wheel and the menu/MID control.

The raw tuple remains authoritative for code.
