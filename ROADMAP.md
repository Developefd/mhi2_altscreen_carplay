# Roadmap and help wanted

This is a research roadmap, not a promise of release dates.

## P0 — make the current lifecycle deterministic

Highest-value work:

- capture clean provider-transition traces;
- close `suggestUI` -> receiver selection behavior;
- determine when `showUI` / `stopUI` is necessary;
- tune the now vehicle-proven same-session `forceKeyFrame` recovery;
- replace the current periodic 1 s D2 watchdog with an event-scoped/bounded watchdog policy;
- preserve the proven VideoConfig/IDR generation rules;
- eliminate stale-last-frame cases without synthetic Type-111 teardown.

## P0 — more vehicles / exact compatibility gates

Wanted:

- Škoda MHI2 testers on other firmware trains;
- SEAT/CUPRA MHI2 testers;
- Volkswagen MHI2 testers;
- different Virtual Cockpit/AID revisions.

SSH access is currently expected via WLAN or USB-LAN.

## P1 — ViewArea / SafeArea / VC layout mapping

Goals:

- read actual stock cluster view state;
- map cluster layout -> project layout abstraction;
- advertise useful ViewAreas;
- request/switch ViewArea in-session;
- validate safe areas against gauge overlays;
- calibrate the first 1010x376 top/bottom SafeArea from vehicle photos;
- later pre-advertise multiple ViewAreas and switch them live through `updateViewArea`.

## P1 — make developer deployment cleaner

Before a polished installer:

- compatibility manifest;
- preflight script;
- exact backup manifest;
- reversible enable/disable;
- crash-safe STOCK fallback;
- log collection;
- target-specific profiles.

## P2 — installer / SD workflow

Only after lifecycle and compatibility are stable.

The current project deliberately prefers visible SSH/developer steps over hiding changing assumptions
behind a one-click SD package.

## P2 — navigation metadata / arrows

Potential later direction:

- RGI / maneuver metadata;
- navigation arrows;
- cluster instruction-card integration.

Public Luka/derived RGI work is the obvious prior-art starting point.

## Good first contributions

You do not need to solve the whole project.

Useful contributions include:

- one clean state trace;
- one new firmware hash + ABI check;
- one provider-switch reproduction;
- one VC layout mapping;
- one QNX/IBM-J9 finding;
- one documentation correction backed by evidence;
- build portability improvements.

See `CONTRIBUTING.md` and use Discussions for exploratory findings before opening a hard bug.


## P1 — steering-wheel / hardkey input for runtime layout control

A useful cross-brand lead is
`y-batsianouski/mib2-voicecontrol-button-patch`, originally tested on VW
`MHI2_ER_VWG13_K4525_MU1367`.

The exact Škoda MU1440 `lsd.jxe` audit confirms the same core ASL input substrate:

- `ASLSystemAPI.addKeyListener(...)`;
- `ASLSystemAPI.createAndSubmitHardkeyEvent(...)`;
- PTT listener key 15;
- the OEM 500 ms `DoublePressKeyAdapter` classifier;
- matching `speechgeneral.ptt.DialogSession` behavior/abort-key layout;
- matching mute/smartphone hardkey model from the exact target key mapping.

This makes button-driven ViewArea/layout switching a credible later path.

Do not copy the external shadow-JAR bootstrap blindly. It uses `-Xbootclasspath/p`; exact-class
shadowing on the reference MU1440 is currently being isolated separately after it disturbed the
native VC map path in another Java experiment.

Preferred first implementation: passive/additive listener observation, then controlled layout
switching; remapping/injection can follow after exact vehicle traces.


### Runtime selector UX

Once SafeArea/ViewArea presets are calibrated, expose them through a developer-facing selector before
attempting a polished end-user UI.

Candidate paths:

- Green Engineering Menu entry for explicit profile/preset selection;
- later steering-wheel/hardkey cycling through the exact ASL listener path;
- file/config control remains the diagnostic fallback.

The initial useful selector set is expected to include navigation composition (`stock`,
`map-rich`, etc.) plus calibrated SafeArea/ViewArea presets.
