#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${1:-}"

if [[ -z "$DEST" ]]; then
  echo "usage: $0 /path/to/mounted-sd-root"
  exit 2
fi

GEN2="$ROOT/artifacts/mu1440/altscreen111/vehicle-tested-2026-09-27/libaltscreen111.so"
REMUX="$ROOT/artifacts/mu1440/direct-ts-remux/vehicle-tested-2026-09-27/direct-ts-remux"
GATE="$ROOT/artifacts/mu1440/isotx2-gate/vehicle-tested-run51/libmibr_isotx2_gate.so"
SHAHELP="$ROOT/artifacts/mu1440/sha256sum-compat/build-confirmed-current/sha256sum"
TEEHELP="$ROOT/artifacts/mu1440/tee-compat/build-confirmed-current/tee"
NAVIGNORE="$ROOT/artifacts/mu1440/navignore/vehicle-tested-2026-09-27/MIBR-NavIgnore.jar"

for f in "$GEN2" "$REMUX" "$GATE" "$SHAHELP" "$TEEHELP" "$NAVIGNORE"; do
  [[ -f "$f" ]] || {
    echo "missing required public artifact: $f"
    echo "If the gate artifact is missing, run/wait for the public support-tools workflow."
    exit 10
  }
done

OUT="$DEST"
mkdir -p "$OUT"

# The developer package intentionally lives directly in the SD-card root.
# Never remove the SD root itself; replace only project-owned deployment paths.
rm -f "$OUT/install.sh" "$OUT/uninstall.sh" "$OUT/status.sh" "$OUT/PAYLOAD.sha256"
rm -rf "$OUT/runtime/auto-direct" "$OUT/runtime/isotx2-gate" "$OUT/runtime/diagnostics" "$OUT/runtime/navigation" "$OUT/runtime/deployment"
mkdir -p "$OUT/payload" "$OUT/runtime/auto-direct" "$OUT/runtime/isotx2-gate" "$OUT/runtime/diagnostics" "$OUT/runtime/navigation" "$OUT/runtime/deployment"

cp "$ROOT/deployment/mu1440/install.sh" "$OUT/install.sh"
cp "$ROOT/deployment/mu1440/uninstall.sh" "$OUT/uninstall.sh"
cp "$ROOT/deployment/mu1440/status.sh" "$OUT/status.sh"
cp "$GEN2" "$OUT/payload/libaltscreen111.so"
cp "$REMUX" "$OUT/payload/direct-ts-remux"
cp "$GATE" "$OUT/payload/libmibr_isotx2_gate.so"
cp "$SHAHELP" "$OUT/payload/sha256sum"
cp "$TEEHELP" "$OUT/payload/tee"
cp "$NAVIGNORE" "$OUT/payload/MIBR-NavIgnore.jar"

cp "$ROOT/runtime/auto-direct/"*.sh "$OUT/runtime/auto-direct/"
cp "$ROOT/runtime/auto-direct/altscreen111.conf" "$OUT/runtime/auto-direct/"
cp "$ROOT/runtime/isotx2-gate/"*.sh "$OUT/runtime/isotx2-gate/"
cp "$ROOT/runtime/diagnostics/gen2_keyframes.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/diagnostics/gen2_resync.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/diagnostics/gen2_status.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/diagnostics/keypanel_trace_discovery.sh" "$OUT/runtime/diagnostics/"
cp "$ROOT/runtime/navigation/gen2_nav_config.sh" "$OUT/runtime/navigation/"
cp "$ROOT/runtime/deployment/session.sh" "$OUT/runtime/deployment/"

chmod +x "$OUT/"*.sh "$OUT/runtime/"*/*.sh "$OUT/payload/libaltscreen111.so" "$OUT/payload/direct-ts-remux" "$OUT/payload/libmibr_isotx2_gate.so" "$OUT/payload/sha256sum" "$OUT/payload/tee"
chmod 644 "$OUT/payload/MIBR-NavIgnore.jar"

(
  cd "$OUT"
  sha256sum payload/libaltscreen111.so payload/direct-ts-remux payload/libmibr_isotx2_gate.so payload/sha256sum payload/tee payload/MIBR-NavIgnore.jar > PAYLOAD.sha256
)

echo "prepared SD root: $OUT"
echo "included NavIgnore: $OUT/payload/MIBR-NavIgnore.jar"
echo "NavIgnore SHA-256: b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34"
