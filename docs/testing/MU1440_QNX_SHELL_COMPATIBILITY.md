# MU1440 QNX shell / command compatibility

Target: `MHI2_ER_SKG13_P4526_MU1440`  
Context: direct SSH / MMX QNX shell

This file records exact-target command constraints that matter to this repository's developer
deployment. It exists because desktop Linux habits are not a safe compatibility model for the unit.

## Operator command rule

For direct SSH copy/paste, use **one logical command per paste**.

Avoid interactive multi-line constructs such as multi-line `awk` programs, shell functions or
unfinished quotes. An incomplete paste can leave the QNX shell at the continuation prompt `>`.
Recover with Ctrl+C before entering another command.

Prepared script files may contain multi-line shell/awk programs; that is a different execution path
and several such exact programs have already been vehicle-tested.

## Confirmed exact-target limitations

### external `printf`

Not available in the tested direct runtime PATH.

Observed:

```text
./gen2_nav_config.sh[459]: printf: cannot execute - No such file or directory
```

Runtime scripts use `echo` for the required simple line writes instead.

### stock `sha256sum`

Do not assume it exists.

The guarded public developer package carries its own reproducibly built QNX ARMv7
`payload/sha256sum` and uses that exact helper by default.

### `sed` / `tr`

Do not make either a vehicle runtime dependency. `sed` is missing on some older firmware trains, so the stable deployment removed its only required use in favor of stock `awk`. Earlier live-monitor work also had to remove `tr`.

### GNU `date` behavior

Do not assume GNU formatting/options. Runtime code uses the known RCC date path only as a best-effort
timestamp source and has a fallback.

### grep alternation

Do not rely on desktop BRE alternation such as:

```text
grep 'A\|B'
```

Use separate fixed-string checks where correctness matters.

## `awk`: important distinction

`awk` is **not banned**.

The exact MU1440 has successfully executed the project’s scripted `awk` transformations in the
vehicle-proven gate/CarPlay tooling, and `gen2_nav_config.sh` used its small status parsers during
the successful live composition tests.

What is not safe is treating an interactive multi-line `awk '...'` paste as an operator-friendly
command. The shell/paste failure that motivated this document happened in that interactive form.

The developer installer now checks that `awk` exists **before persistent mutation**.

## Proven direct-shell patterns

Examples repeatedly used on the reference vehicle:

```sh
cd /net/mmx/fs/sda0/esd/carplay-test
cp source destination
mv source destination
rm -f file
touch file
cat file
grep -n -F 'literal' file
grep '^prefix=' file
pidin fds 2>/dev/null | grep 'isoTX2'
mount -uw /net/mmx/fs/sda0/
mount -ur /net/mmx/fs/sda0/
mount -uw /mnt/app
mount -ur /mnt/app
sync
sleep 1
```

QNX `on -d -f mmx` is vehicle-proven inside the audited Auto-Direct scripts.

## SD and persistent mount state

Readable is not writable.

The SD card may return read-only after reboot. Persistent application/system filesystems are also
normally read-only. Every mutator must remount the filesystem it owns and restore the expected
read-only state after the write.

Do not make a previous menu action or manual remount an implicit prerequisite.

## Standalone developer-package dependency model

The prepared SD package is now independent of an existing M.I.B. card layout:

- no `config/BASICS` / GLOBALS bootstrap;
- no `apps/mounts` helper;
- no `/apps/sbin/sha256sum` fallback;
- no firmware/M.I.B. `tee` dependency; the package carries its own QNX ARMv7 `payload/tee`.

The project-supplied command-line compatibility binaries required by the deployment are `payload/sha256sum` and `payload/tee`. The native project payload also contains the feature binaries
`libaltscreen111.so`, `direct-ts-remux` and `libmibr_isotx2_gate.so`, but these are functional
components rather than replacements for missing shell utilities.

Mandatory firmware/QNX commands used by the install/runtime path are fail-closed in the top-level
preflight: `mount cp mv chmod sync mkdir rm touch sleep grep awk wc cat pidin on slay`, plus
`/bin/sh`, `/bin/ksh` and `/eso/bin/apps/dmdt`.

Some diagnostic-only paths also try tools such as `tail`, `netstat`, `ls`, `uname`, `use` or
`strings`. Those calls are optional/fallback diagnostics and are not installation prerequisites.

## Developer-package preflight

Before changing persistent files, `deployment/mu1440/install.sh --check` validates:

- bundled SHA-256 helper;
- exact MU1440 `libairplay.so` hash;
- exact published GEN2/remux/gate hashes;
- required runtime files;
- the exact command set used by the current install/runtime;
- inventory/archive of conflicting or foreign Java bootclasspath state before any replacement;
- DisplayManager gate patchability;
- exact NavIgnore presence/hash, either from `payload/` or an already-installed target copy.

Unknown state is fail-closed.

## Regression rule

A runtime/deployment shell change should not introduce:

- external `printf`;
- bare dependency on firmware `sha256sum` or `tee`;
- a dependency on M.I.B. `config/BASICS`, `apps/mounts` or `/apps/sbin`;
- an interactive multi-line patch procedure as the normal install path;
- a persistent mutation before tool/hash/target preflight;
- an assumption that a readable SD or persistent filesystem is writable.

The public CI checks the first deployment-level regressions automatically.
