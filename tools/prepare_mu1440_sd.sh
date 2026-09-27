#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${1:-}"

if [[ -z "$DEST" ]]; then
  echo "usage: $0 /path/to/sd/esd"
  exit 2
fi

GEN2="$ROOT/artifacts/mu1440/altscreen111/vehicle-tested-2026-09-27/libaltscreen111.so"
REMUX="$ROOT/artifacts/mu1440/direct-ts-remux/vehicle-tested-2026-09-27/direct-ts-remux"
GATE="$ROOT/artifacts/mu1440/isotx2-gate/build-confirmed-current/libmibr_isotx2_gate.so"

for f in "$GEN2" "$REMUX" "$GATE"; do
  [[ -f "$f" ]] || {
    echo "missing required public artifact: $f"
    echo "If the gate artifact is missing, run/wait for the public support-tools workflow."
    exit 10
  }
done

OUT="$DEST/MHI2AltScreen"
rm -rf "$OUT"
mkdir -p "$OUT/payload" "$OUT/runtime"

cp "$ROOT/deployment/mu1440/install.sh" "$OUT/install.sh"
cp "$ROOT/deployment/mu1440/uninstall.sh" "$OUT/uninstall.sh"
cp "$ROOT/deployment/mu1440/status.sh" "$OUT/status.sh"
cp "$GEN2" "$OUT/payload/libaltscreen111.so"
cp "$REMUX" "$OUT/payload/direct-ts-remux"
cp "$GATE" "$OUT/payload/libmibr_isotx2_gate.so"

mkdir -p "$OUT/runtime/auto-direct" "$OUT/runtime/isotx2-gate" "$OUT/runtime/diagnostics" "$OUT/runtime/navigation"
cp "$ROOT/runtime/auto-direct/"*.sh "$OUT/runtime/auto-direct/"
cp "$ROOT/runtime/auto-direct/altscreen111.conf" "$OUT/runtime/auto-direct/"
cp "$ROOT/runtime/isotx2-gate/"*.sh "$OUT/runtime/isotx2-gate/"
cp "$ROOT/runtime/diagnostics/gen2_keyframes.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/diagnostics/gen2_resync.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/diagnostics/gen2_status.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/navigation/gen2_nav_config.sh" "$OUT/runtime/navigation/"
cp "$ROOT/runtime/navigation/gen2_safearea.sh" "$OUT/runtime/navigation/"

chmod +x "$OUT/"*.sh "$OUT/runtime/"*/*.sh "$OUT/payload/libaltscreen111.so" "$OUT/payload/direct-ts-remux" "$OUT/payload/libmibr_isotx2_gate.so"

(
  cd "$OUT"
  sha256sum payload/libaltscreen111.so payload/direct-ts-remux payload/libmibr_isotx2_gate.so > PAYLOAD.sha256
)

echo "prepared: $OUT"
echo "optional: place the separately obtained exact MIBR-NavIgnore.jar in $OUT/payload/"
