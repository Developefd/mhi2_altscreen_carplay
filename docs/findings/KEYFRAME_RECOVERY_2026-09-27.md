# Same-stream keyframe recovery — vehicle finding 2026-09-27

## Summary

The 2026-09-27 vehicle run reproduced a stale Virtual Cockpit frame while the complete media
transport continued to make progress.

This was not a stopped Stream-111 failure.

## Failure discriminator

At the stale frame:

```text
Stream 111        streaming
source AUs        increasing
direct remux      running
MOST blocks       increasing
MOST write errors 0
VC                visually frozen
```

The stream, codec and consumer generations remained unchanged.

## Manual recovery proof

Before the first manual recovery:

```text
source_idrs=2
```

A serialized same-session `forceKeyFrame` produced:

```text
source_idrs=3
resync_completions=1
```

and the VC immediately resumed moving video.

The stale condition was reproduced again. A second manual request produced:

```text
source_idrs=4
resync_completions=2
```

and again restored moving video without rebuilding Stream 111.

This is repeated vehicle evidence that the observed lifecycle failure is a decoder/presentation
synchronization problem on an otherwise live transport.

## Why an IDR matters here

H.264 P/B pictures depend on reference pictures. A receiver that loses the usable reference chain
during a provider/UI/composition transition can continue receiving access units while being unable
to resume correct presentation.

An IDR picture creates a new random-access point: the decoder can discard the old reference state and
start from the new intra-coded picture.

The CarPlay `forceKeyFrame` control maps to the sender-side bitstream restart/keyframe mechanism, so
the recovery can happen on the existing Stream-111 session.

## D2 automatic policy result

After the manual proof, D2 was enabled.

A later status snapshot showed:

```text
d2_event_triggers=21
d2_watchdog_triggers=241
resync_requests=258
resync_completions=258
source_idrs=260
```

The tested Apple Maps / Google Maps / Waze transitions then remained usable without manual recovery.

### Retry-counter note

The vehicle snapshot also reported a very high cumulative `resync_retries` value. That counter was
misleading in the tested build: it treated almost every lifetime request after the first as a retry,
even when it was the first request of a new resync epoch. The development source now scopes the
retry test to the current epoch by checking whether that epoch already issued a request.

This is a telemetry correction only; it does not change the vehicle-proven keyframe recovery behavior.

## Why the current watchdog still needs tuning

The current implementation intentionally treats "no newer source IDR for 1000 ms" as a watchdog
condition.

After each forced IDR, the timer is refreshed. If the sender does not naturally produce another IDR
within the next second, the watchdog requests another one. Therefore the current policy behaves
approximately like a periodic 1 Hz IDR refresher while D2 is enabled.

That is useful for proving the synchronization hypothesis, but it is more aggressive than required.

The intended next policy is:

```text
relevant lifecycle/composition event
        |
        +-- debounce
        |
        +-- request one fresh IDR
        |
        +-- wait for confirmed newer source IDR
        |
        +-- disarm watchdog
             until the next event or real stale detector
```

A slower emergency safety watchdog may remain, but the normal path should not request an IDR every
second indefinitely.

## Relevant events

Current useful triggers include:

- `turns` state transitions;
- `suggestUI` changes;
- explicit navigation surface changes;
- provider ownership/start/stop transitions where visible;
- a future positive stale-frame detector.

The key point is to keep Stream 111 alive and refresh the bitstream only when synchronization needs
to be re-established.
