# Exact MU1440 steering-wheel / hardkey API audit

## External lead

Reference project:

`y-batsianouski/mib2-voicecontrol-button-patch`

That project is documented as tested on VW:

`MHI2_ER_VWG13_K4525_MU1367`

It remaps the MFW voice-control button by combining an OEM speech-session bootstrap with normal ASL
key listeners and synthetic hardkey events.

## Exact Škoda MU1440 result

The reference project's mechanism was compared against the exact
`MHI2_ER_SKG13_P4526_MU1440` `lsd.jxe`.

Confirmed exact-target APIs include:

```text
ASLSystemAPI.addKeyListener(int, KeyListener)
ASLSystemAPI.addKeyListener(int, int, KeyListener)
ASLSystemAPI.createAndSubmitHardkeyEvent(int, int)

KeyAdapter.onPressed(int)
KeyAdapter.onReleased(int)
KeyAdapter.onLongPressed(int)

DoublePressKeyAdapter.onDoublePressed(int)
DoublePressKeyAdapter.onSingleReleased(int)
```

The exact MU1440 `DoublePressKeyAdapter` contains the same 500 ms single/double-press classifier
expected by the external project.

The exact MU1440 `speechgeneral.ptt.DialogSession` also has the same relevant behavior:

- extends `DoublePressKeyAdapter`;
- PTT logic uses key ID 15;
- same `onDoublePressed` / `onSingleReleased` structure;
- same `ABORT_KEYS` content.

Previously recovered exact-target key mapping evidence also matches the external implementation's
important DSI hardkeys, including mute 88 and smartphone/App-Connect 114.

## Interpretation

This is strong evidence that the **ASL input/hardkey substrate is portable between the tested VW
MHI2 implementation and the Škoda MU1440 reference target**.

It is not yet a vehicle-compatibility claim for the complete external patch.

The external implementation bootstraps itself by shadowing OEM `DialogSession` through
`-Xbootclasspath/p`. The current project has separately observed that exact-stock bootclasspath
shadowing of certain MOST/Kombi classes can disturb native VC initialization on MU1440.

Therefore the preferred project path is:

```text
exact API audit
  -> passive/additive key listener
  -> observe real MFW events
  -> optional ViewArea/layout switching
  -> only then consider remap/injected hardkeys
```

If an additive initialization point can be found, it is preferable to another OEM-class shadow for
the first vehicle experiment.

## Potential use in this project

A steering-wheel event could eventually provide a user-facing runtime selector for:

- ViewArea/SafeArea preset;
- navigation composition profile;
- VC layout preset;
- diagnostics/test mode.

This is complementary to a Green Engineering Menu selector; it does not need to replace one.
