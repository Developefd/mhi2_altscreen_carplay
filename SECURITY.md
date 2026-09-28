# Security and privacy

This project modifies infotainment/display behavior on vehicle hardware. Treat all experimental builds as potentially disruptive and all vehicle logs/screenshots as potentially identifying.

## Public posts: redact first

Before posting an Issue, Discussion, log, screenshot or trace, remove information that is not needed to reproduce the technical result.

Do not publish:

- credentials, passwords, tokens or private SSH keys;
- Wi-Fi credentials or other network secrets;
- VINs or unnecessary vehicle/account identifiers;
- home, work or other personal/navigation addresses;
- precise location data that is not technically required;
- personal filenames, usernames or private infrastructure details;
- unpublished third-party proprietary source code.

Navigation screenshots deserve particular care because map content, favorites, recent destinations or GPS state can disclose precise locations. Logs, counters and hashes are often better evidence.

If sensitive information is accidentally posted, remove/redact it promptly and rotate any exposed credential that may still be valid.

## Security-sensitive findings

If you discover a vulnerability that would materially enable unauthorized access to vehicles or systems, do not publish exploit details in a normal Issue or Discussion.

Prefer GitHub **Private vulnerability reporting** when available for this repository. Otherwise contact the maintainer privately through the GitHub account profile/contact channel first so the finding can be handled responsibly.

Ordinary project bugs, compatibility failures and reproducible vehicle regressions should still use the normal process described in [SUPPORT.md](SUPPORT.md).

## Scope

The current project is focused on:

- interoperability;
- display/video routing;
- CarPlay auxiliary-screen behavior;
- Virtual Cockpit presentation;
- reproducible research on owned/test hardware.

It is not intended as a repository for theft-enablement, credential bypasses or unauthorized vehicle access.

## Vehicle experiments

Before testing a runtime change:

1. verify exact firmware/build compatibility;
2. preserve a recovery route;
3. avoid testing while driving;
4. start from a known stock baseline;
5. collect sanitized logs;
6. confirm that stock behavior can be restored.

No experimental result in this repository should be interpreted as a safety certification.
