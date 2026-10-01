# CarPlay cluster function / frontend-effect matrix — iOS 26.7.1

Date: 2026-10-01  
Current public target: **iOS 26.7.1 (23H30)**  
Legacy project target: Harman MHI2 auxiliary screen / Stream 111

This is the second-pass deep dive behind the broader
[CarPlay cluster / AltScreen configuration index](CARPLAY_CLUSTER_CONFIGURATION_INDEX_IOS26_7_1_2026-10-01.md).

The purpose of this document is to answer a narrower question:

> If a CarPlay cluster setting, command or presentation mode exists, what does it actually change in the
> visible frontend, which URL/view does it belong to, and how strong is the evidence?

The main correction from earlier research is that several similarly named controls live on different
planes. In particular there are **three unrelated zoom systems**:

1. semantic cluster **map zoom** (`changeMapZoomLevel`);
2. Apple-internal cluster zoom Scene Actions (`CRSUIClusterZoomAction`);
3. whole-CarPlay **Smart Display Zoom / display scaling** (`displayScaleMode` + `ZoomFactor`).

They must not be mixed.

---

## Evidence legend

| Mark | Meaning |
| --- | --- |
| **O** | Official Apple public documentation / WWDC behavior. |
| **P** | Proven in a concrete Apple binary/decompiled call path. |
| **W** | Proven/corroborated at the AirPlay/CarPlay wire or receiver-SDK level. |
| **D** | Device-proven in an independent receiver implementation. |
| **I** | Inference from names/layout state; useful, but exact visible behavior is not fully pinned. |

For legacy MHI2, **test now** means the function can be exercised without replacing the current
Stream-111 H.264 transport architecture. It does not mean a particular OEM cluster is guaranteed to
honor the result.

---

## 1. Current iOS 26.7.1 checkpoint

The public `blacktop/ipsw-diffs` comparison for:

```text
iOS 26.7 (23H24) -> iOS 26.7.1 (23H30)
```

shows one updated shared-cache dylib:

```text
/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics
```

No change is listed for DashBoard, CarKit, CarPlay.framework, CarPlayUIServices, AirPlaySender or
AVRouting.

Therefore **26.7.1 does not add a visible CarPlay control-surface delta relative to 26.7** in that
public diff. This does **not** prove that every intermediate 26.x release was CarPlay-identical; the
known 26.x evolution is summarized later.

Evidence:

- https://support.apple.com/149226
- https://github.com/blacktop/ipsw-diffs/tree/main/iOS/26_7_23H24_vs_26_7_1_23H30

---

## 2. Frontend-effect matrix

| Function / parameter | Plane | View / URL / scope | Visible frontend effect | Evidence | Legacy MHI2 |
| --- | --- | --- | --- | --- | --- |
| `maps:/car/instrumentcluster` | classic cluster URL | base instrument-cluster URL | Generic / AnyContent navigation cluster presentation. Template host selects the navigation controller rather than map-only or turn-card-only content. | **P** | **test now** |
| `.../map` | classic cluster URL | `maps:/car/instrumentcluster/map` | Map presentation. Template host removes the generic navigation/turn-card child and adds the map/ETA view. | **O+P** | **test now** |
| `.../instructioncard` | classic cluster URL | `maps:/car/instrumentcluster/instructioncard` | Maneuver / guidance-card presentation instead of the map view. | **O+P** | **test now** |
| `showETA=yes/no/user` | classic URL setting | base/map cluster URL | Controls ETA content. Template host directly calls `setShowETA:` on map ETA tray and navigation controller when setting resolves to “yes”. | **O+P** | **test now** |
| `showSpeedLimit=yes/no/user` | classic URL setting | base/map cluster URL | Controls speed-limit sign/indicator visibility in the navigation cluster UI. Maps consumes the resulting `showsSpeedLimit` scene setting. | **O+P** | **test now** |
| `showCompass=yes/no/user` | classic URL setting | base/map cluster URL | Controls compass/heading indicator visibility. Maps consumes `showsCompass`; hybrid navigation controller exposes heading-indicator state. | **O+P** | **test now** |
| `maneuverLayout=topAligned/leftAligned/rightAligned` | classic + hosted URL setting | navigation / map-hosted views | Repositions/justifies maneuver/guidance content. Template host maps values to `layoutOverride`; Maps also turns this into hybrid-cluster alignment/map inset decisions. | **P** | **test now** |
| `itemType` | Apple cluster scene setting | derived from classic URL or hosted `mapsPresentation` | Switches among map (1), instruction card (2) and generic/AnyContent navigation (3). Causes actual child-view-controller replacement. | **P** | indirect via URL |
| `changeMapZoomLevel` | AirPlay `/command` | cluster display UUID; map content | Zooms the **map camera/detail level only** on the cluster. Official Apple says vehicles with cluster map-zoom controls should add CarPlay map zoom. Wire direction values in the public receiver path: **0=in, 1=out**. | **O+W** | **highest-priority test** |
| `CRSUIClusterZoomAction` | Apple Scene Action | hosted instrument-cluster scene | Internal semantic zoom event delivered to the hosted cluster app. Apple-internal enum is **1=in, 2=out**, deliberately different from the AirPlay wire enum. | **P** | do not emit as wire command |
| `showUI` | AirPlay UI control | display UUID + optional approved cluster URL | Foregrounds/presents the selected cluster content on the named display. Public receiver research matches Apple cluster-picker behavior. | **W** | **test now** |
| `stopUI` | AirPlay UI control | display UUID | Hides/stops the current UI on the named display. Established stop shape is UUID-only; do not rely on a URL argument. | **W** | **test now** |
| `suggestUI` | AirPlay UI suggestion | candidate URL set | Soft/candidate presentation signal rather than a transport teardown. iOS sender research shows candidate withdrawal can be `suggestUI([])`. | **P+W** | log first |
| `requestUI` | AirPlay UI request | session/UI context | Requests the other side to bring UI forward. It is **not** a Home-button synonym and is distinct from `showUI`. Exact caller direction is path-dependent. | **W** | bounded experiment |
| `requestViewArea` | AirPlay ViewArea control | screen/display UUID + declared ViewArea index | Requests transition to another predeclared display region/layout without creating a new screen stream. | **P+W+D** | **high-priority test** |
| `updateViewArea` | AirPlay ViewArea control | same existing display; declared ViewArea index | Applies the requested layout. Wire includes `uuid`, `viewAreaIndex`, `animationDurationMillis`, `adjacentViewAreas`; geometry comes from predeclared `viewAreas[]`. Independent hardware work shows coded video can remain constant while the visible region moves/animates. | **W+D** | **high-priority test** |
| `viewAreas[]` / `safeArea` | display advertisement | Stream-111 display | Defines allowable rendered regions and protected content rectangle. Changes composition/cropping/insets, not the semantic map camera. | **O+W+D** | **test with two areas** |
| `initialViewArea` / `adjacentViewAreas` | display advertisement | Stream-111 display | Chooses starting layout and which ViewArea transitions are exposed/allowed. | **W+D** | **test with ViewArea** |
| `updateDisplayPanels` | modern AirPlay display control | display-panel topology | Live panel reconfiguration: geometry, pixel/physical dimensions, UUID, input model, maxFPS, initial URL/ViewArea, streams, appearance and zoom/display properties. Broader and riskier than a legacy ViewArea switch. | **P+W** | later A/B |
| `uiAppearanceUpdate` | AirPlay appearance | named display | Changes CarPlay UI/chrome appearance (light/dark/tint-style state) on an enabled display. | **P+W** | later, capability-gated |
| `mapAppearanceUpdate` | AirPlay appearance | map display | Changes map-specific appearance independently of general UI appearance. | **P+W** | later, capability-gated |
| `setNightMode` | AirPlay session/display state | global/advisory | Day/night switch that can influence UI and map styling. It is not the same as per-display map appearance. | **W** | available |
| `mapStyle` | Apple Scene setting | CarPlay scene | Internal resolved map-style state consumed by CarPlay scenes. Frontend result is map style/theme; do not conflate it with the accessory command name. | **P** | observe rather than inject |
| `forceKeyFrame` | AirPlay screen control | existing screen stream | No intentional layout change; forces a fresh H.264 keyframe/bitstream refresh. Frontend effect is recovery from stale/frozen video, not a new presentation mode. | **P+W** | **test/recovery primitive** |
| `maxFPS` | display advertisement | individual display/Stream 111 | Advertises the receiver/display frame-rate ceiling. It can limit source cadence but does not itself describe per-frame timing. | **W** | test independently |
| `frameRateLimit` | Apple Scene setting | instrument-cluster scene | Limits/schedules Apple-side scene rendering; tied to scene-diff and thermal policy. Exists since at least iOS 17.6.1. No classic accessory setter is proven. | **P** | observe only |
| `CRSUIClusterPressAction` | Apple Scene Action | hosted Apple cluster apps | Semantic select/button action delivered to hosted cluster apps. iOS 18.2 explicitly gates press types against Apple CarTrip/CarTirePressure bundle IDs. Not a proven AirPlay `pressType` command. | **P** | do not invent wire packet |
| Knob / D-Pad HID | HID / AirPlay report | display-bound HID device | Physical selection/rotation/nudge/Home/Back input where declared. Useful for classic cluster interaction, but separate from `CRSUIClusterPressAction`. | **W** | only after correct HID advertisement |
| `nextGenHostedContent:` | Apple private hosted URL | hosted/gauge-cluster scene | Entry point for next-generation hosted content. Parser exists, but classic `isURLSupported:` continues to validate `maps:/car/instrumentcluster...`; therefore this is not automatically a legacy `showUI` URL. | **P** | gate unresolved |
| `mapsPresentation=map` | hosted URL setting | `nextGenHostedContent:/maps/...` | Hosted Maps content is map-centric. Resolves to Maps presentation/item type 1. | **P** | next-gen gate unresolved |
| `mapsPresentation=instructionCard` | hosted URL setting | hosted Maps | Hosted maneuver/guidance-card content. Resolves to item type 2. | **P** | next-gen gate unresolved |
| `mapsPresentation=anyContent` | hosted URL setting | hosted Maps | Generic hosted navigation content. Resolves to item type 3. | **P** | next-gen gate unresolved |
| `altScreenPresentation=dcaCarousel` | next-gen hosted | cluster DCA carousel | Real carousel presentation. CarPlayAssetUI has an active DCA carousel item and rotation model; Trip supports `clusterDCA`. Maps has DCA-specific guidance-card sizing. Exact expansion of “DCA” is not established here. | **P** | **not classic-111 proven** |
| `altScreenPresentation=mapsMediaCarousel` | next-gen hosted | cluster Maps+Media carousel | **Joint Maps + Media cluster mode.** Maps returns `isRenderedInMediaCarousel=YES` for presentation type 2; Media sets `isClusterMapsAndMedia = (type == 2)` and logs “Radio cluster maps and media”. Exact spatial arrangement remains asset/OEM dependent. | **P, very strong** | **not classic-111 proven** |
| `altScreenPresentation=popover` | next-gen hosted | cluster popover | Dedicated cluster-popover presentation. Trip recognizes `clusterPopover`; CarPlayAssetUI contains `PopoverModel` / `PopoverView` and transition coordination. Visible behavior is a transient/floating cluster content surface, but precise geometry is asset/OEM dependent. | **P + I geometry** | **not classic-111 proven** |
| `altScreenPresentation=passengerDisplay` | next-gen hosted | passenger-display scene | Routes content to a **separate passenger display**, not simply another cluster card. Media explicitly creates/stores a passenger scene/window for type 4. | **P** | not our VC target |
| `altScreenPresentation=deck` | next-gen hosted (26.2+) | generic hosted content | Generic deck/card/carousel composition. 26.2 adds `deck`, `DeckActivity`, carousel direction/model data, left/right carousel swipe handlers and widget-host migration. Exact final OEM presentation and enum value are not pinned from the 26.1 parser. | **P existence + I exact UI** | not legacy proven |
| `displayLocation` | Apple Scene setting | hosted scene topology | Chooses where the hosted scene belongs. iOS 26.1 logging resolves values including **Center Console**, **Passenger Display** and **Secondary Cluster**. This is topology, not a map style. | **P** | not classic wire |
| Smart Display Zoom / `displayScaleMode` | CarKit + CarPlay Settings | whole CarPlay display | **Scales the entire CarPlay UI**, allowing a denser/smaller or larger UI depending on supported screen configuration. Apple says apps are automatically resized to the new display scale. This is not map zoom. | **O+P** | main/display-scaling research, not map-zoom command |
| display `ZoomFactor` / `zoomFactor` | display configuration / CarKit | display-scale computation | Numeric input to CarKit display scaling. iOS 26.1 reads `ZoomFactor`; scaling logs use roughly `preferred-to-original scale ratio / ZoomFactor`. Later 26.x propagates zoom factor deeper into session-host configuration. Not a semantic map zoom. | **P** | investigate separately |
| `enablesViewAreas` | capability gate | display/session | Enables ViewArea negotiation/switching. Without the gate, declaring rectangles alone is not sufficient on modern stacks. | **W+D** | required for modern behavior |
| `enablesMapAppearance` | capability gate | display/session | Arms map appearance control. | **W** | later |
| `enablesUIAppearance` | capability gate | display/session | Arms UI appearance control. | **W** | later |
| `enablesFocusTransfer` / `viewAreaSupportsFocusTransfer` | capability/input focus | multi-surface UI | Allows focus movement between CarPlay and native surfaces. Independent receiver evidence shows this is **orthogonal to ViewArea acceptance**. | **W+D** | defer |
| `changeUIContext` / UI-context URL sets | AirPlay context | multi-surface UI | Transfers logical UI context among eligible surfaces/URLs. Distinct from simple show/stop and largely irrelevant until the richer UI-context feature is advertised. | **W** | defer |

---

## 3. Why `changeMapZoomLevel` was easy to miss

It sits between the layers previously searched:

```text
classic URL parser
    └─ showETA / showSpeedLimit / showCompass / maneuverLayout

AirPlay command plane
    └─ changeMapZoomLevel { uuid, zoomDirection }

Apple hosted scene plane
    └─ CRSUIClusterZoomAction
```

So a search centered on cluster URLs will not find the wire command, while a search centered on
CarPlayUIServices finds an internal Scene Action with a **different enum**.

This distinction is important enough to pin:

| Zoom mechanism | Scope | Values |
| --- | --- | --- |
| AirPlay `changeMapZoomLevel` | map content on cluster | 0 = zoom in, 1 = zoom out |
| `CRSUIClusterZoomAction` | Apple-internal hosted Scene Action | 1 = zoom in, 2 = zoom out |
| Smart Display Zoom | whole CarPlay UI/display scale | display-scale mode, not directional map zoom |
| display `ZoomFactor` | display-scaling heuristic | numeric scale input |

Apple also documents the user-facing requirement publicly: a vehicle that exposes map zoom controls for
its instrument cluster should implement CarPlay map zoom.

Official source:

- https://developer.apple.com/videos/play/wwdc2023/10150/

---

## 4. `mapsMediaCarousel`: exact current interpretation

The earlier conclusion can now be strengthened.

### Maps side

`CarInstrumentClusterChromeConfiguration`:

- reads `hostedAltScreenPresentationType`;
- returns true from `isRenderedInMediaCarousel` when presentation type is **2**;
- consumes ETA, speed-limit, compass and layout state;
- feeds hybrid instrument-cluster map/navigation controllers.

### Media side

The Media app:

- accepts hosted presentation types 1..3 as **cluster scenes**;
- accepts type 4 as a separate **passenger display scene**;
- when cluster scene settings change, sets:
  `isClusterMapsAndMedia = (hostedAltScreenPresentationType == 2)`;
- logs the state as:
  `Radio cluster maps and media`.

This makes the functional interpretation unusually strong:

> **`mapsMediaCarousel` is an Apple first-party cluster composition in which Maps and Media are both
> participants in the same hosted-cluster presentation.**

What remains unknown is **the exact OEM-visible geometry**: which side/card is larger, whether album art
or metadata is shown in a particular slot, and what cluster chrome surrounds the content. Those details
are driven by CarPlayAssetUI/vehicle assets and layout configuration.

It is therefore not safe to translate this directly into “send
`nextGenHostedContent...?altScreenPresentation=mapsMediaCarousel` over classic MHI2 `showUI`”.

---

## 5. Hosted presentation family

The 26.1 parser maps:

```text
dcaCarousel       -> 1
mapsMediaCarousel -> 2
popover           -> 3
passengerDisplay  -> 4
```

All 1..3 are treated by Media as cluster-scene presentations; type 4 is passenger-display-specific.

### DCA carousel

Concrete evidence:

- CarPlayAssetUI: “Setting new active DCA carousel item”;
- carousel model/direction;
- Trip: `CAFUIAppPresentationMode.clusterDCA`;
- Maps: dedicated DCA-carousel guidance-card sizing.

Result:

> **Proven carousel presentation; exact meaning of the “DCA” acronym is intentionally left unresolved.**

### Popover

Concrete evidence:

- Trip: `CAFUIAppPresentationMode.clusterPopover`;
- TirePressure selects a popover UI configuration for hosted type 3;
- CarPlayAssetUI contains `PopoverModel`, `PopoverView`, `PopoverContainer` and transition handling.

Result:

> **Proven cluster popover surface.** It is reasonable to describe the visible behavior as
> transient/floating content rather than a full persistent map pane, but exact placement is vehicle-asset
> dependent.

### Passenger display

Media contains separate passenger-scene and passenger-window state and logs a new passenger display
scene when hosted presentation type is 4.

Result:

> **Proven separate passenger display destination**, not a special MHI2 VC map mode.

### Deck — post-26.1

The 26.1 -> 26.2 public diff adds:

```text
nextGenHostedContent:/?altScreenPresentation=deck
deck
DeckActivity
deckDataByID
Carousel
CarouselModel.Direction
handleCarouselSwipeLeftGesture
handleCarouselSwipeRightGesture
```

along with widget-host migration and ViewArea-state strings.

Result:

> High-confidence **generic hosted deck/card/carousel composition**, broader than Maps. Exact visual
> semantics and numeric enum are not pinned by the 26.1 parser and should remain marked inferred until a
> complete 26.2+ call path is reconstructed.

---

## 6. Smart Display Zoom: a separate control family we had not indexed before

This is a significant second-pass addition.

The iOS 26.1 CarPlay Settings app contains:

```text
SMART_DISPLAY_ZOOM_CELL_TITLE
CPSettingsSmartDisplayZoomToggle
```

and shows the control only when a connected screen reports that Smart Zoom is allowed.

CarKit `CRDisplayScaleInfo` exposes:

```text
allowsSmartZoom
canvasPixelSizeForDisplayScaleMode:
displayScaleModesForCanvasPixelSize:
defaultDisplayMode
optimizedPointScale
originalPointScale
preferredPointScale
zoomFactor
```

It reads `ZoomFactor` from AirPlay display configuration and computes alternative canvas sizes.

Apple publicly describes the visible iOS 26 behavior as a CarPlay Settings option that adjusts display
scale; apps are automatically resized to the new scale.

This is therefore **not**:

- cluster map zoom;
- a MapKit camera control;
- the same thing as ViewArea cropping.

A 26.3 -> 26.4 carkitd diff also adds explicit propagation of:

```text
displayScaleMode
zoomFactor
```

into `CARSessionRequestHost`, showing that the display-scaling path continued to evolve after 26.1.

Official source:

- https://developer.apple.com/videos/play/wwdc2025/216/

---

## 7. iOS 26.x evolution after the 26.1 full-tree baseline

The second pass did **not** surface a new classic Stream-111 cluster command after
`changeMapZoomLevel` / ViewArea / appearance / UI-control families already known.

It did surface continued evolution in the hosted/display layer:

| Range | Relevant change |
| --- | --- |
| 26.1 -> 26.2 | `deck` hosted presentation; deck/carousel asset model; carousel swipe handlers; widget-host migration; new ViewArea state strings. |
| 26.2 / 26.3 | CarPlayAssetUI deck/carousel/slot composition continues to evolve. |
| 26.3 -> 26.4 | DashBoard ViewArea update paths change; CarKit/carkitd deepen `displayScaleMode` + `zoomFactor` session handling. |
| 26.5 -> 26.6 betas | Appearance-resolution and AVRouting/ViewArea internals change; no new classic cluster command name was found in the reviewed diffs. |
| 26.7 -> 26.7.1 | No CarPlay-related dylib delta in public 23H24 -> 23H30 diff; CoreGraphics is the only updated shared-cache dylib. |

Do not interpret the table as proof that every private implementation detail stayed byte-identical
across all intermediate builds. It describes the **publicly visible diff evidence** relevant to this
research.

---

## 8. Practical MU1440 test order

The shortest path from research to visible vehicle evidence remains:

```text
1. changeMapZoomLevel
2. classic URL / itemType A/B:
      base
      map
      instructioncard
3. showETA / showSpeedLimit / showCompass
4. maneuverLayout top / left / right
5. two declared ViewAreas
6. requestViewArea / updateViewArea
7. appearance controls
8. maxFPS A/B while measuring real AU cadence
9. updateDisplayPanels as a separate experiment
10. nextGenHostedContent only after a real hosted capability/scene gate is identified
```

Do **not** combine in one experiment:

- map zoom and Smart Display Zoom;
- `maxFPS` and Apple scene `frameRateLimit`;
- ViewArea switching and `updateDisplayPanels`;
- classic `maps:/car/instrumentcluster...` URLs and private `nextGenHostedContent:` URLs.

---

## 9. Source anchors

### Apple public

- WWDC19 — Advances in CarPlay Systems:
  https://developer.apple.com/videos/play/wwdc2019/252/
- WWDC23 — Optimize CarPlay for vehicle systems:
  https://developer.apple.com/videos/play/wwdc2023/10150/
- WWDC25 — Turbocharge your app for CarPlay:
  https://developer.apple.com/videos/play/wwdc2025/216/

### Full public iOS restore trees

- `SuperChaoM/iPhone15-3_17.6.1_21G101_Restore@95df6b804bee0e01538eb2ddad0413609938d063`
- `EthanArbuckle/iPhone17-1_18.2_22C152_Restore@e26ed4563f78871c59d2d96856756a65d62517e5`
- `EthanArbuckle/iPhone18-3_26.1_23B85_Restore@90aa0cfe59d9682b4265e1354c8b19ec3c7823ab`

High-value 26.1 files:

```text
DashBoard/DBInstrumentClusterURLHandler.mm
CarPlayUIServices/CRSUIInstrumentClusterSceneSettings.mm
CarPlayUIServices/CRSUIMutableInstrumentClusterSceneSettings.mm
CarPlayUIServices/CRSUIClusterZoomAction.mm
CarPlayUIServices/CRSUIClusterZoomBSActionsHandler.mm
CarPlayUIServices/CRSUIClusterPressAction.mm
CarPlayTemplateUIHost/CARTemplateUIHostInstrumentClusterViewController.mm
Maps/CarInstrumentClusterChromeConfiguration.mm
Maps/CarHybridInstrumentClusterNavigationModeController.mm
Maps/UIWindow.mm
Media/Media_07.mm
CarKit/CRDisplayScaleInfo.mm
CarPlaySettings/CARDisplayPanel.mm
```

### Public diff evidence

- `blacktop/ipsw-diffs`
- especially 18.2 -> 18.3 hosted-content evolution;
- 26.1 -> 26.2 `deck` / carousel additions;
- 26.3 -> 26.4 display-scale / ViewArea changes;
- 26.7 -> 26.7.1 current no-CarPlay-dylib delta.

### Receiver / wire cross-reference

- `45clouds/WirelessCarPlay@51145ef55f8dd9f1cbadd58353cacb5e0ca215e9`
- `lvalen91/ocbm` — command and ViewArea ground-truth work
- `omonob/MHI2-Carplay-Maps@49882eb56db83a7b42f185adb426161f35a08295`
- `arctus/mib2-carplay-rgi-altscreen@404ccc23450e385a0183d37fc93677a0c9a5637d`
- `luka-dev/mib2q-carplay-rgi`

---

## 10. Research rule

For a new function, keep these questions separate:

1. Does a string/class/key exist?
2. Does Apple write/read it in a real call path?
3. Does it select a real frontend component?
4. Is there an AirPlay wire command?
5. Is the wire command accessory-reachable?
6. Is a capability/URL allowlist required?
7. Can a legacy type-111 receiver advertise that capability?
8. Does the MU1440 vehicle visibly honor it?

This separation is specifically what prevents internal Scene Actions, next-generation hosted URLs and
classic AirPlay commands from being merged into one misleading “CarPlay parameter” list.
