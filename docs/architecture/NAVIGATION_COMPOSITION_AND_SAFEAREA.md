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

There are two distinct mechanisms.

### Changing the definition

Changing the rectangle/SafeArea definition itself requires a new display-info negotiation. LIVI does
the same in practice: its `applyDisplayConfig()` only updates local configuration and its
`clusterViewArea*` / `clusterSafeArea*` settings are marked restart-required while projection is
active.

### Selecting an already-declared ViewArea

CarPlay also supports a real in-session ViewArea transition. The receiver first advertises multiple
ViewAreas, each with its own geometry and nested SafeArea, and then selects among them by index:

```text
type = updateViewArea
params.uuid                    = <display UUID>
params.viewAreaIndex           = <declared index>
params.animationDurationMillis = <duration>
params.adjacentViewAreas       = [ ... ]
```

The command does **not** redefine a rectangle. It selects a preset that was negotiated earlier.

Therefore the intended runtime design is:

1. advertise several calibrated ViewArea/SafeArea presets during initial negotiation;
2. provide valid adjacency metadata;
3. select the appropriate preset in-session through `updateViewArea`;
4. later bind that index to the physical VC layout once a reliable layout-state signal is identified.

The current GEN2 implementation contains an `updateViewArea` command scaffold, but currently
advertises only one ViewArea and does not yet serialize the full adjacency list. Multi-ViewArea
switching is therefore protocol-understood but not yet vehicle-proven on MU1440.

See
[CarPlay cluster / Ultra research closeout](../research/CARPLAY_CLUSTER_ULTRA_CLOSEOUT_2026-10-03.md)
for the final research status.
