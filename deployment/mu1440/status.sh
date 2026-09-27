#!/bin/ksh
set -u

SELF=$0
case "$SELF" in */*) ROOT=${SELF%/*} ;; *) ROOT=. ;; esac
ROOT=$(cd "$ROOT" 2>/dev/null && pwd) || exit 2

DST=/mnt/app/root/altscreen-u2
echo "=== MHI2 AltScreen developer status ==="

if [ -x "$DST/scripts/direct_ts_auto_status.sh" ]; then
  "$DST/scripts/direct_ts_auto_status.sh"
else
  echo "auto_direct_status=missing"
fi

echo
if [ -x "$DST/scripts/gen2_keyframes.sh" ]; then
  "$DST/scripts/gen2_keyframes.sh" status
else
  echo "gen2_keyframes=missing"
fi

echo
if [ -x "$DST/scripts/gen2_nav_config.sh" ]; then
  "$DST/scripts/gen2_nav_config.sh" status
else
  echo "gen2_nav_config=missing"
fi

echo
echo "safearea_runtime=NOT_IN_STABLE_DEPLOYMENT_PAYLOAD"
echo "note=SafeArea-capable GEN2 remains a separate vehicle candidate until validated"

echo
if [ -x "$ROOT/runtime/isotx2-gate/verify_preload_boot.sh" ]; then
  MIBR_SHA256="$DST/bin/sha256sum" \
    MIBR_GATE_CONTROL="$ROOT/runtime/isotx2-gate/gate.sh" \
    MIBR_PRELOAD_CONTROL="$ROOT/runtime/isotx2-gate/preload_control.sh" \
    "$ROOT/runtime/isotx2-gate/verify_preload_boot.sh"
fi

echo
if [ -x "$DST/scripts/verify_carplay111_boot.sh" ]; then
  "$DST/scripts/verify_carplay111_boot.sh"
fi
