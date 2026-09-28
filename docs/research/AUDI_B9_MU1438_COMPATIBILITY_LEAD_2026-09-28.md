# Audi B9 / AUG22 MU1438 compatibility lead — 2026-09-28

Status: **external evidence / read-only compatibility research**  
Reference implementation remains: **Škoda MHI2 / MU1440 + AID10-class**

## Why this target is interesting

A contributor investigating an Audi B9 MHI2 / AUG22 MU1438 target reported strong structural similarity around the classic Harman CarPlay receiver. A supplied stock-configuration bundle adds useful independent evidence about the process and display topology.

This does **not** mean the current MU1440 binary should be loaded on MU1438. The existing exact-hash gate remains correct until ABI and vehicle routing are proven.

## Contributor-supplied stock configuration evidence

Bundle SHA-256:

`13533cff4a68dee5d39c097ea60c2012c0baef78a646287ab91fc12419d69a2e`

| File | SHA-256 |
| --- | --- |
| `dio_manager.json` | `99a11a14efa4a02bc5811b5d29d0b336695089c06774ca4568bb7844f4688528` |
| `smartphone_integrator.json` | `6e7968e0279a2548c50285923e5bbc66f8775ad490c2394e430318d570faeed9` |
| `displaymanager.json` | `c2263eb38ff03dbd23d1c95ddf47e04762df97e40da1825f8677801cec24d644` |

### CarPlay process model

`smartphone_integrator.json` launches CarPlay as:

```text
smartphone_integrator -> dio_manager
```

with `IPL_CONFIG_DIR_DIO_MANAGER=/etc/eso/production`, decoder use enabled and the stock CarPlay cleanup script.

### Screen receiver

`dio_manager.json` configures:

- maximum 30 fps;
- NVIDIA secondary-screen output selection;
- output device 0 = LVDS (default), 1 = HDMI;
- bad-frame watchdog threshold;
- the stock comment notes the dependency chain from the initial I-frame for subsequent frames.

### MOST video capability

`displaymanager.json` contains:

- MOST encoder device `/dev/mlb/isoTX2`;
- Audi-oriented queue size 8;
- encoder/channel-rate parameters;
- `video_over_most.force_routing = component_control`.

This is directly relevant to the MU1440 reference path, which also uses `/dev/mlb/isoTX2`.

### Second LVDS / HDMI capability

The same DisplayManager config also exposes an extended `2_lvds` mode:

- primary terminal: `Tegra:TFTLCD0`;
- secondary terminal: `Tegra:HDMI0`;
- secondary display: ID 4;
- HBAS test/production profiles force `kombi_type = lvds`.

Therefore the evidence does **not** justify reducing the architecture to "Audi uses LVDS, Škoda uses MOST". Both mechanisms are present in the Audi stock configuration.

## Main unknown: exact cluster and productive route

For the concrete Audi B9 vehicle, we still need to identify:

- exact Virtual Cockpit / instrument-cluster part number;
- HW/SW identification and generation;
- native resolution;
- stock navigation DisplayManager context/display IDs;
- whether the live cluster map/video path is carried by MOST, the second LVDS/HDMI terminal, or a combination of both;
- how ownership/arbitration differs from the MU1440/AID10 reference target.

This is likely one of the principal portability gates.

## Binary / ABI gate

The contributor also reported promising binary-level similarities around the classic Harman AirPlay/CarPlay implementation, including matching-looking hook-site prologues and relevant Screen/AES/SendCommand symbol families.

Those observations are useful for prioritizing the target, but they are **not** yet sufficient to declare ABI compatibility. The next safe step is an exact read-only comparison of:

1. stock `libairplay.so` identity/hash;
2. target functions and prologue bytes;
3. call signatures / relocation dependencies;
4. required DSI/DisplayManager interfaces;
5. process-local dependencies used by the MU1440 hook.

Only after that comparison should a target-specific build be considered.

## Admission criteria

Audi B9 / AUG22 MU1438 can move from **compatibility lead** to a vehicle-test candidate only after:

- [ ] exact firmware + stock component hashes captured;
- [ ] exact cluster HW/SW identity captured;
- [ ] productive cluster transport identified;
- [ ] hook/ABI comparison passes;
- [ ] target-specific build is produced rather than bypassing the MU1440 gate;
- [ ] reversible STOCK recovery is defined;
- [ ] first vehicle experiment is bounded and logged.

Until then, the correct status is **interesting and structurally promising, but unvalidated**.
