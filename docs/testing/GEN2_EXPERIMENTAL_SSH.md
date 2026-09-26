# Experimental GEN2 SSH workflow

> Developer workflow only. No SD-card installer is provided.

This guide is intentionally conservative. The published snapshot assumes that the target already has
a compatible preload/hook arrangement for:

```text
/mnt/app/eso/lib/libmibr_carplay111.so
```

and that the existing smartphone integration configuration loads that path.

If you do not already have that prerequisite, do not copy the binary blindly. Start with the
architecture/source documentation instead.

## 1. Verify the exact firmware component

Reference target:

```text
MHI2_ER_SKG13_P4526_MU1440
```

Expected `libairplay.so` SHA-256:

```text
193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
```

Target path:

```text
/mnt/app/eso/lib/libairplay.so
```

Stop if the hash differs.

## 2. Verify the experimental binary

Expected SHA-256:

```text
f3efa9f09972422307f1b9e37e323539036d935f9cc630a1ed7d8f223a2bbae2
```

Verify on the workstation and again on the unit after transfer.

## 3. Stage by SSH/SCP

Use a generic staging directory of your choice, for example:

```text
/mnt/app/root/mibr-gen2-stage
```

The repository deliberately does not require an SD-card path.

Keep the staged binary separate from the active hook until all prerequisites and backup hashes have
been checked.

## 4. Preserve rollback authority

Before replacing an active hook:

- record its SHA-256;
- copy it to a dedicated backup path;
- record the backup SHA-256 next to it;
- verify that the backup is readable before making the active filesystem writable.

A tested project convention is:

```text
/mnt/app/root/mibr-gen2-backup/
```

Do not overwrite an existing backup whose provenance you do not understand.

## 5. Existing preload prerequisite

The tested arrangement expects the smartphone integration configuration to already reference:

```text
LD_PRELOAD=/mnt/app/eso/lib/libmibr_carplay111.so
```

The experimental binary replaces that project hook, not the stock Apple/Harman `libairplay.so`
itself.

## 6. Reboot and observe

After an intentional hook replacement, reboot into a known state and check:

- CarPlay main screen still works;
- Type-111 status becomes active when navigation starts;
- a valid VideoConfig is seen;
- H.264 access units advance;
- downstream Direct-VC transport receives data;
- stock recovery remains possible.

Do not test while driving.

## 7. Provider-change test

A useful sequence for this specific snapshot is:

```text
Apple Maps navigation
  -> confirm live VC video
  -> end/switch provider
  -> observe whether last frame freezes
  -> attempt same-session reacquire
  -> if needed, reconnect CarPlay cable
  -> confirm live video returns
```

This snapshot is published precisely because that behavior is known and understandable.

## 8. Current development differs

Do not use this binary to conclude that the current development line still has exactly the same
behavior.

Current development is further ahead in automatic start/stop, provider switching and same-session
recovery logic. See `docs/status/CURRENT_DEVELOPMENT_STATUS.md`.

## 9. Restore before experimenting further

If behavior becomes unclear:

- restore the exact pre-test hook from the verified backup;
- reboot;
- confirm normal CarPlay and stock VC behavior;
- only then start another experiment.

Recovery is part of the test result.
