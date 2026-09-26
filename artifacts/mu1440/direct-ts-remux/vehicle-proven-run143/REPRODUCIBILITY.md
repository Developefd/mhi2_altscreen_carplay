# direct-ts-remux reproducibility note

## Canonical vehicle-tested developer binary

The canonical binary committed in this directory is an unstripped QNX/ARM developer rebuild of the
vehicle-tested downstream implementation.

SHA-256:

```text
672275fc0e840e604cef68b6bd7b1e9e2059342471997d9bd859e32c34a0f02b
```

Properties:

- QNX 6.5 ARMv7 / EABI5;
- `-O2 -g`;
- not stripped;
- `.symtab` retained;
- `.debug_info` retained;
- source published at `src/native/direct-ts-remux/direct_ts_remux.c`.

## Independent public rebuild

The repository can independently rebuild the same public source using the pinned public
`luka-dev/qnx65-armv7-toolchain`.

Pinned toolchain source commit:

```text
56a66557245af14077678cd28a83ce3a337d9e2d
```

The independent rebuild produced:

```text
e55f387bcc0d0995fc1221c85fea5af09292914138f6354b656a007bae7c8263
```

The total unstripped ELF is not byte-identical to the canonical binary.

A binary comparison showed that the relevant runtime/code regions matched while differences were
introduced in DWARF build/debug metadata and the resulting section-header placement. This is
consistent with different build paths/environment metadata rather than a different remuxer
implementation.

For this reason:

```text
672275fc... = canonical vehicle-tested developer binary
e55f387b... = independent public rebuild
```

The public CI is a reproducibility check. It does not overwrite the canonical vehicle-tested binary.

## Source identity

The public source file itself is the authority for future rebuilds.

If that source changes, a new binary should not be described as the existing vehicle-tested baseline
without another explicit vehicle validation.
