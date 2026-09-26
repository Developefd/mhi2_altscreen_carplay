# Security and responsible reporting

This project modifies infotainment/display behavior on vehicle hardware. Treat all experimental builds as potentially disruptive.

## Please do not publish

Do not post:

- credentials;
- private SSH keys;
- Wi-Fi passwords;
- VINs or other unnecessary vehicle identifiers;
- personal addresses visible in navigation screenshots/logs;
- secrets from private infrastructure;
- unpublished third-party proprietary source code.

Redact logs before attaching them.

## Security-sensitive findings

If you discover a vulnerability that would materially enable unauthorized access to vehicles or systems, do not publish exploit details in a normal Issue.

Contact the maintainer privately through the GitHub account profile/contact channel first so the finding can be handled responsibly.

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
5. collect logs;
6. confirm that stock behavior can be restored.

No experimental result in this repository should be interpreted as a safety certification.
