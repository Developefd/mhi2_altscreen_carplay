# Navigation composition, ViewArea and SafeArea

## Logical UI surfaces

The current vehicle tests support three distinct CarPlay cluster roles on the same secondary-display
transport:

```text
maps:/car/instrumentcluster
maps:/car/instrumentcluster/map
maps:/car/instrumentcluster/instructioncard
```

They are presentation contexts, not three independent Stream-111 sessions.

## Query-controlled composition

Current reverse engineering and vehicle tests show that the base/map URLs can carry composition
parameters such as:

```text
showETA
showSpeedLimit
showCompass
maneuverLayout
```

Waze in particular exposed additional lower trip-summary content when the richer query set was
enabled. Left/right maneuver layout changed the reserved panel position and moved the visual map
center.

## SafeArea model

GEN2 advertises a full logical display of:

```text
1010 x 376
```

The vehicle-tested binary used that complete rectangle as the nested SafeArea as well.

The current development source adds one persistent SafeArea config:

```text
/mnt/app/root/mibr-carplay111-safearea.conf
```

Format:

```text
x=0
y=0
w=1010
h=376
```

Invalid or out-of-range geometry falls back to the full canvas.

The first calibration target is a top/bottom inset while leaving the full left/right width available.

## Apply model

The initial low-risk implementation applies changed geometry on the next AltScreen display-info
handshake. A CarPlay reconnect is therefore the first apply mechanism; a complete MU reboot should
not be required as the first step.

## Later live switching

The control plane already contains `updateViewArea(viewAreaIndex)`.

The intended runtime design is:

1. advertise several calibrated ViewArea/SafeArea presets during initial negotiation;
2. select the appropriate preset in-session through `updateViewArea`;
3. later bind that index to the physical VC layout once a reliable layout-state signal is identified.

This avoids mutating an already-advertised rectangle in place and follows the existing protocol
mechanism.
