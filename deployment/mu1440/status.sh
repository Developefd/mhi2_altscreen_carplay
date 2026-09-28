#!/bin/ksh
set -u

SELF=$0
case "$SELF" in */*) ROOT=${SELF%/*} ;; *) ROOT=. ;; esac
ROOT=$(cd "$ROOT" 2>/dev/null && pwd) || exit 2

DST=/mnt/app/root/altscreen-u2
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
NAVJAR=/mnt/app/eso/hmi/lsd/jars/MIBR-NavIgnore.jar
EXPECTED_NAVIGNORE=b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34
SHA="$DST/bin/sha256sum"
echo "=== MHI2 AltScreen developer status ==="

echo
echo "=== Java / NavIgnore ==="
NAVREFS=0
[ -r "$LSD" ] && NAVREFS=$(awk 'index($0,"MIBR-NavIgnore.jar"){n++} END{print n+0}' "$LSD" 2>/dev/null)
echo "navignore_bootclasspath_refs=$NAVREFS"
if [ -r "$NAVJAR" ] && [ -x "$SHA" ]; then
  set -- $("$SHA" "$NAVJAR" 2>/dev/null)
  NH=${1:-}
  echo "navignore_sha256=${NH:-UNKNOWN}"
  [ "$NH" = "$EXPECTED_NAVIGNORE" ] && echo "navignore_state=PASS_EXACT" || echo "navignore_state=FAIL_HASH"
elif [ "$NAVREFS" -gt 0 ]; then
  echo "navignore_state=FAIL_BOOTCLASSPATH_WITHOUT_JAR"
else
  echo "navignore_state=ABSENT"
fi

if [ -r "$LSD" ]; then
  grep -Fq 'MIBR-DirectVCPolicy.jar' "$LSD" 2>/dev/null && echo "java_conflict=DirectVCPolicy"
  grep -Fq 'Most20FPS.jar' "$LSD" 2>/dev/null && echo "java_conflict=Most20FPS"
  grep -Fq 'NavActiveIgnore.jar' "$LSD" 2>/dev/null && echo "java_conflict=NavActiveIgnore_legacy"
fi

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
