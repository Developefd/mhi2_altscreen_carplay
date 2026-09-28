#!/bin/ksh
set -u

SELF=$0
case "$SELF" in */*) ROOT=${SELF%/*} ;; *) ROOT=. ;; esac
ROOT=$(cd "$ROOT" 2>/dev/null && pwd) || exit 2

DST=/mnt/app/root/altscreen-u2
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
PAYLOAD=$ROOT/payload
SHA=${MIBR_SHA256:-$PAYLOAD/sha256sum}
TEE=${MIBR_TEE:-$PAYLOAD/tee}
SESSION_HELPER=$ROOT/runtime/deployment/session.sh

[ -r "$SESSION_HELPER" ] || { echo "MIBR_UNINSTALL=FAIL missing_session_helper=$SESSION_HELPER"; exit 20; }
. "$SESSION_HELPER" || { echo "MIBR_UNINSTALL=FAIL cannot_source_session_helper"; exit 20; }

if [ "${MIBR_LOG_ACTIVE:-0}" != "1" ]; then
  [ -x "$SHA" ] || { echo "MIBR_UNINSTALL=FAIL missing_sha256_helper=$SHA"; exit 20; }
  [ -x "$TEE" ] || { echo "MIBR_UNINSTALL=FAIL missing_tee_helper=$TEE"; exit 20; }
  mibr_prepare_session uninstall "$ROOT" "$TEE" "$SHA" || {
    echo "MIBR_UNINSTALL=FAIL cannot_create_sd_issue_log rc=$?"
    exit 20
  }
  echo "Logging complete uninstall session to $MIBR_LOG_FILE"
  mibr_run_logged "$0" "$@"
  exit $?
fi

mibr_vehicle_summary || echo "WARN vehicle_summary_failed"

OWNED=/mnt/app/root/mibr-deploy-navignore-owned
TMP=/tmp/lsd.sh.mibr-deploy-navignore-remove.$$

FAIL=0

echo "=== MHI2 AltScreen MU1440 developer uninstall ==="
[ -x "$SHA" ] || { echo "MIBR_UNINSTALL=FAIL missing_sha256_helper=$SHA"; exit 20; }

if [ -x "$DST/scripts/direct_ts_auto_disable.sh" ]; then
  "$DST/scripts/direct_ts_auto_disable.sh" || FAIL=1
fi
if [ -x "$DST/scripts/direct_ts_auto_stop.sh" ]; then
  "$DST/scripts/direct_ts_auto_stop.sh" || FAIL=1
else
  rm -f /tmp/mibr-isotx2-gate.direct 2>/dev/null || true
fi

if [ -e "$OWNED" ]; then
  echo "Removing deployment-owned NavIgnore"
  if mount -uw /mnt/app 2>/dev/null; then
    awk '
      $0 == "# MIBR NAVIGNORE" { next }
      index($0, "-Xbootclasspath/p:$BASE_DIR/lsd/jars/MIBR-NavIgnore.jar") { next }
      { print }
    ' "$LSD" > "$TMP" &&
    chmod 755 "$TMP" 2>/dev/null &&
    mv "$TMP" "$LSD"
    RC=$?
    if [ "$RC" -eq 0 ]; then
      rm -f /mnt/app/eso/hmi/lsd/jars/MIBR-NavIgnore.jar "$OWNED" 2>/dev/null || true
    else
      rm -f "$TMP" 2>/dev/null || true
      FAIL=1
    fi
    sync
    mount -ur /mnt/app 2>/dev/null || true
  else
    FAIL=1
  fi
else
  echo "NavIgnore not owned by deployment; leaving it untouched."
fi

echo "Restoring DisplayManager startup"
MIBR_SHA256="$SHA" "$ROOT/runtime/isotx2-gate/restore_preload.sh" || FAIL=1

echo "Restoring stock smartphone_integrator configuration"
if [ -x "$DST/scripts/restore_stock.sh" ]; then
  "$DST/scripts/restore_stock.sh"
  RC=$?
  case "$RC" in 0|21) ;; *) FAIL=1 ;; esac
else
  echo "WARN internal restore_stock.sh missing"
  FAIL=1
fi

rm -f /tmp/mibr-alt111-keyframe-policy.enabled /tmp/mibr-alt111-resync.enabled /tmp/mibr-alt111-resync-arm 2>/dev/null || true

if [ "$FAIL" -ne 0 ]; then
  echo "MIBR_UNINSTALL=COMPLETED_WITH_ERRORS"
  exit 20
fi

echo "MIBR_UNINSTALL=PASS"
echo "REBOOT_REQUIRED=YES"
echo "Runtime/evidence files under $DST are intentionally retained."
exit 0
