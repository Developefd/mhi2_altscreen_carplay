# Publication privacy policy

This repository deliberately separates technical reproducibility from personal provenance.

## What is intentionally not published

- home/work/navigation screenshots;
- street names or route history;
- VINs;
- private hostnames;
- personal e-mail addresses;
- local workstation usernames/paths;
- private repository/account names;
- private CI registry names;
- credentials, SSH keys or Wi-Fi information.

## Binary audit

Project-owned binaries intended for public download are checked for obvious identifying strings before
publication.

Generic build paths such as:

```text
/vehicle
/core
/tmp/gccbuild
```

may remain when they are toolchain/build-context paths and do not identify a person or location.

## Screenshots

Vehicle/navigation screenshots are omitted by default because even technically useful images often
contain:

- precise map location;
- recognizable street names;
- recent destinations;
- home/work area;
- other vehicle/user context.

Prefer:

- text logs;
- counters;
- hashes;
- state-machine traces;
- ASCII diagrams;
- cropped/sanitized technical captures only when they add evidence that cannot be conveyed otherwise.

## Provenance

Public provenance should identify:

- public source path;
- artifact SHA-256;
- firmware/component compatibility hash;
- public third-party dependency;
- public toolchain revision;
- validation class.

It should not require publishing the maintainer's private research-account identity or local machine
layout.

## Before making the repository public

Perform both:

1. current-tree privacy scan;
2. public-history/actions-log cleanup.

The repository should not rely on "the sensitive file was deleted later" as a privacy strategy.
