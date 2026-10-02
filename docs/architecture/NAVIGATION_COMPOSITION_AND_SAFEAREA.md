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

### Initial MU1440 two-preset contract

The first implementation target is deliberately **exactly two** declared ViewAreas, each with its
own SafeArea:

```text
ViewArea 0  MAP_FULL
  viewArea  = calibrated map/full-layout rectangle
  safeArea  = calibrated SafeArea 0
  adjacent  = [1]

ViewArea 1  GAUGE_REDUCED
  viewArea  = calibrated gauge/reduced-layout rectangle
  safeArea  = calibrated SafeArea 1
  adjacent  = [0]

initialViewArea = 0
```

The outer Stream-111 coded/display baseline remains **1010x376**. For the first vehicle calibration
the two ViewAreas may retain the same outer rectangle and differ only in their nested SafeAreas; if
the vehicle trace proves that the OEM layouts also require different ViewArea geometry, those
rectangles can be calibrated independently without changing the two-index control model.

No coordinates beyond the already proven outer canvas are frozen here: the two SafeArea rectangles
are vehicle-calibration outputs, not values to guess offline.

The intended runtime sequence is therefore:

1. calibrate SafeArea 0 for the map/full VC layout;
2. calibrate SafeArea 1 for the gauge/reduced VC layout;
3. advertise both ViewAreas during initial negotiation;
4. advertise adjacency `0 -> [1]` and `1 -> [0]`;
5. start with `initialViewArea=0`;
6. select 0/1 in-session through `updateViewArea`;
7. bind that index to a proven physical VC layout/button state without consuming the OEM button
   behavior.

The current GEN2 implementation contains an `updateViewArea` command scaffold, but currently
advertises only one ViewArea and does not yet serialize the full adjacency list. Multi-ViewArea
switching is therefore protocol-understood but not yet vehicle-proven on MU1440.

See
[CarPlay cluster / Ultra research closeout](../research/CARPLAY_CLUSTER_ULTRA_CLOSEOUT_2026-10-03.md)
for the final research status.
