# Navigation composition vehicle matrix — 2026-09-27

These observations were made on the real MU1440/Virtual-Cockpit path. Most tests were stationary, so
speed-limit and maneuver behavior still need a controlled driving pass.

| Profile / control | Apple Maps | Google Maps | Waze | Current conclusion |
| --- | --- | --- | --- | --- |
| **stock** | normal cluster map | baseline not separately characterized | normal map; no rich lower summary observed | bare `maps:/car/instrumentcluster` baseline |
| **base-rich** | richer instruction/composition behavior | not cleanly isolated yet | arrival time, remaining time and remaining distance visible | query parameters affect real composition |
| **base-left** | limited stationary effect | open | info/maneuver area moves left; map center shifts right | `maneuverLayout` works |
| **base-right** | limited stationary effect | open | info/maneuver area moves right; map center shifts left | `maneuverLayout` works |
| **base-top** | not conclusively characterized | open | needs actual maneuver while driving | open driving test |
| **map-rich** | visibly wider / more zoomed-out map | open | open | `/map` has different map framing |
| **map-clean** | visually close to stock while stationary | open | open | `/map` with extras disabled |
| **maneuver-only** | dark/gray instruction-only surface with maneuver text | open | black / no useful presentation in tested state | `/instructioncard` is not a normal map |
| **compass** | provider attribution needs one clean repeat | small compass observed on one Maps provider | not observed | provider-dependent |
| **speed limit** | inconclusive while stationary | inconclusive | inconclusive while stationary | test on a road with known provider speed-limit data |

## Vehicle-observed query controls

The current development line exposes:

```text
showETA=yes|no|user
showSpeedLimit=yes|no|user
showCompass=yes|no|user
maneuverLayout=leftAligned|rightAligned|topAligned
```

and these surfaces:

```text
maps:/car/instrumentcluster
maps:/car/instrumentcluster/map
maps:/car/instrumentcluster/instructioncard
```

## SafeArea observation

The rich Waze layout exposes useful ETA/time/distance information at the lower edge of the secondary
display. With the full 1010x376 canvas advertised as the SafeArea, part of this provider UI can fall
under the physical VC overlay.

The next development source therefore adds configurable SafeArea geometry. It is build-tested but
not yet part of the vehicle-tested binary checkpoint.
