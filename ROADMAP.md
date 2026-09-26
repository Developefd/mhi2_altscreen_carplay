# Roadmap and help wanted

This is a research roadmap, not a promise of release dates.

## P0 — make the current lifecycle deterministic

Highest-value work:

- capture clean provider-transition traces;
- close `suggestUI` -> receiver selection behavior;
- determine when `showUI` / `stopUI` is necessary;
- classify same-session `forceKeyFrame` recovery;
- prove the correct VideoConfig/IDR generation rules;
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
- validate safe areas against gauge overlays.

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
