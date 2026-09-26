# Research and tooling methodology

This project combines conventional reverse engineering with automation-assisted analysis.

Work has included:

- static binary analysis and symbol/xref inspection;
- vehicle-side runtime traces;
- controlled A/B tests;
- protocol/lifecycle reconstruction;
- reproducible CI builds;
- source-level instrumentation;
- comparison against public prior art;
- **AI-assisted reverse engineering, code review, hypothesis generation and documentation**.

AI-assisted work is treated as an engineering aid, not as evidence by itself.

A claim is promoted to a project finding only when it is supported by one or more concrete evidence
classes such as:

- vehicle observation;
- exact binary evidence;
- reproducible build/test result;
- independently corroborated protocol behavior.

This distinction is intentional because generated hypotheses can be useful for choosing the next
experiment but must not be confused with measured behavior.
