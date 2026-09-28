#!/bin/ksh
set -u

SELF=$0
case "$SELF" in
  */*) ROOT=${SELF%/*} ;;
  *) ROOT=. ;;
esac
ROOT=$(cd "$ROOT" 2>/dev/null && pwd) || exit 2

PAYLOAD=$ROOT/payload
RUNTIME=$ROOT/runtime
SHA=${MIBR_SHA256:-$PAYLOAD/sha256sum}
TEE=${MIBR_TEE:-$PAYLOAD/tee}
MEDIA_ROOT=$ROOT
SESSION_HELPER=$RUNTIME/deployment/session.sh

[ -r "$SESSION_HELPER" ] || { echo "MIBR_INSTALL=FAIL missing_session_helper=$SESSION_HELPER"; exit 20; }
. "$SESSION_HELPER" || { echo "MIBR_INSTALL=FAIL cannot_source_session_helper"; exit 20; }

if [ "${MIBR_LOG_ACTIVE:-0}" != "1" ]; then
  [ -x "$SHA" ] || { echo "MIBR_INSTALL=FAIL missing_sha256_helper=$SHA"; exit 20; }
  [ -x "$TEE" ] || { echo "MIBR_INSTALL=FAIL missing_tee_helper=$TEE"; exit 20; }
  mibr_prepare_session install "$MEDIA_ROOT" "$TEE" "$SHA" || {
    echo "MIBR_INSTALL=FAIL cannot_create_sd_issue_log rc=$?"
    exit 20
  }
  echo "Logging complete install session to $MIBR_LOG_FILE"
  mibr_run_logged "$0" "$@"
  exit $?
fi

mibr_vehicle_summary || echo "WARN vehicle_summary_failed"

MEDIA_ROOT=$ROOT
MEDIA_RW=0
DST=/mnt/app/root/altscreen-u2
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
APP_RW=0

media_rw(){
  [ "$MEDIA_RW" -eq 1 ] && return 0
  case "$MEDIA_ROOT" in
    /net/mmx/fs/*)
      mount -uw "$MEDIA_ROOT" 2>/dev/null || return 1
      ;;
  esac
  TEST="$MEDIA_ROOT/.mibr-altscreen-write-test.$$"
  touch "$TEST" 2>/dev/null || return 1
  [ -f "$TEST" ] || return 1
  rm -f "$TEST" 2>/dev/null || return 1
  MEDIA_RW=1
  return 0
}

app_rw(){
  [ "$APP_RW" -eq 1 ] && return 0
  mount -uw /mnt/app 2>/dev/null || return 1
  APP_RW=1
  return 0
}

app_ro(){
  if [ "$APP_RW" -eq 1 ]; then
    sync 2>/dev/null || true
    mount -ur /mnt/app 2>/dev/null || true
    APP_RW=0
  fi
}

cleanup(){
  app_ro
}
trap cleanup 0 1 2 15

EXPECTED_AIRPLAY=193a4fd9101ec2aa05e7159cfa307b96500810d379ca74a194f172adc13a46b5
EXPECTED_GEN2=094e3f1abfbf949f8c11b27e048e8213e5fc71178e62efd3c56deed8ee3bf8d8
EXPECTED_REMUX=b761a8741682e3cbc6e05f5fe1705475c34c4805d1cd1ce09333274a301e735e
EXPECTED_GATE=05673010a88c25022145ffb4e75d3715eaf686f4127ac188e91a52f512b9d957
EXPECTED_NAVIGNORE=b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34
EXPECTED_MOST20=dbd45609fe4ba69948d39e9e649b224484f680f6aa7934b68c261a4d360ea5bb
EXPECTED_LSD_JXE=a55d9cfb69c5756f8202b7f7aa4079d4d5b637ae4c2fd0fe723f1d6816cbeea8

hashf(){
  set -- $("$SHA" "$1" 2>/dev/null)
  [ -n "${1:-}" ] || return 1
  echo "$1"
}

fail(){
  echo "MIBR_INSTALL=FAIL $*"
  app_ro
  exit 20
}

find_cmd(){
  C=$1
  case "$C" in
    */*) [ -x "$C" ] && { echo "$C"; return 0; } ;;
    *)
      OLDIFS=$IFS
      IFS=:
      for D in /proc/boot:/bin:/usr/bin:/usr/sbin:/sbin:/mnt/app/armle/bin:/mnt/app/armle/usr/bin; do
        [ -n "$D" ] || D=.
        if [ -x "$D/$C" ]; then
          IFS=$OLDIFS
          echo "$D/$C"
          return 0
        fi
      done
      IFS=$OLDIFS
      ;;
  esac
  return 1
}

require_cmds(){
  for C in "$@"; do
    find_cmd "$C" >/dev/null 2>&1 || fail "missing_required_command=$C"
  done
}

check_hash(){
  F=$1
  E=$2
  [ -r "$F" ] || fail "missing=$F"
  H=$(hashf "$F") || fail "hash_failed=$F"
  [ "$H" = "$E" ] || fail "hash_mismatch=$F expected=$E actual=$H"
  echo "PASS hash $F $H"
}

java_scan_state(){
  JARDIR=/mnt/app/eso/hmi/lsd/jars
  NAVJAR=$JARDIR/MIBR-NavIgnore.jar
  MOSTJAR=$JARDIR/MIBR-Most20FPS.jar
  JAVA_CONFLICT=0
  JAVA_NEEDS_PAYLOAD=0
  JAVA_OLD_DYNAMIC=0
  JAVA_EXACT_NAV_ACTIVE=0
  JAVA_EXACT_MOST20_ACTIVE=0
  JAVA_REASON=""

  for SPEC in "NavIgnore:MIBR-NavIgnore.jar:$EXPECTED_NAVIGNORE" "Most20:MIBR-Most20FPS.jar:$EXPECTED_MOST20"; do
    NAME=${SPEC%%:*}
    REST=${SPEC#*:}
    JARNAME=${REST%%:*}
    EXPECT=${REST#*:}
    REFS=$(awk -v n="$JARNAME" 'index($0,n){c++} END{print c+0}' "$LSD" 2>/dev/null)
    case "$REFS" in
      0)
        [ -r "$PAYLOAD/$JARNAME" ] || fail "${NAME}_required expected=$EXPECT"
        H=$(hashf "$PAYLOAD/$JARNAME") || fail "${NAME}_hash_failed"
        [ "$H" = "$EXPECT" ] || fail "${NAME}_hash_mismatch=$H"
        grep -q '^\$J9' "$LSD" 2>/dev/null || fail "${NAME}_lsd_insertion_point_missing"
        echo "INFO ${NAME}_payload=valid insertion_point=PASS"
        ;;
      1)
        JAR=/mnt/app/eso/hmi/lsd/jars/$JARNAME
        [ -r "$JAR" ] || fail "${NAME}_bootclasspath_without_jar"
        H=$(hashf "$JAR") || fail "installed_${NAME}_hash_failed"
        [ "$H" = "$EXPECT" ] || fail "installed_${NAME}_hash_mismatch=$H"
        echo "INFO ${NAME}=already_installed hash=$H"
        ;;
      *)
        fail "${NAME}_duplicate_bootclasspath_refs=$REFS"
        ;;
    esac
  done

  echo "PREFLIGHT=PASS"
}

stage_runtime(){
  echo "=== stage internal runtime ==="
  app_rw || fail "mount_app_rw"
  mkdir -p "$DST/bin" "$DST/scripts" "$DST/config" "$DST/logs" "$DST/backup" || fail "runtime_dirs"

  cp "$PAYLOAD/libaltscreen111.so" "$DST/bin/libaltscreen111.so.new" || fail "copy_gen2"
  chmod 755 "$DST/bin/libaltscreen111.so.new" 2>/dev/null || true
  mv "$DST/bin/libaltscreen111.so.new" "$DST/bin/libaltscreen111.so" || fail "install_gen2_stage"

  cp "$PAYLOAD/direct-ts-remux" "$DST/bin/direct-ts-remux.new" || fail "copy_remux"
  chmod 755 "$DST/bin/direct-ts-remux.new" 2>/dev/null || true
  mv "$DST/bin/direct-ts-remux.new" "$DST/bin/direct-ts-remux" || fail "install_remux_stage"

  cp "$SHA" "$DST/bin/sha256sum.new" || fail "copy_sha256"
  chmod 755 "$DST/bin/sha256sum.new" 2>/dev/null || true
  mv "$DST/bin/sha256sum.new" "$DST/bin/sha256sum" || fail "install_sha256"

  cp "$TEE" "$DST/bin/tee.new" || fail "copy_tee"
  chmod 755 "$DST/bin/tee.new" 2>/dev/null || true
  mv "$DST/bin/tee.new" "$DST/bin/tee" || fail "install_tee"

  for F in common.sh patch_carplay.sh restore_stock.sh verify_carplay111_boot.sh direct_ts_auto_enable.sh direct_ts_auto_disable.sh direct_ts_auto_start.sh direct_ts_auto_stop.sh direct_ts_auto_status.sh direct_ts_auto_supervisor.sh direct_ts_auto_watchdog.sh writev_gate.sh; do
    [ -r "$RUNTIME/auto-direct/$F" ] || fail "missing_runtime_$F"
    cp "$RUNTIME/auto-direct/$F" "$DST/scripts/$F.new" || fail "copy_$F"
    chmod 755 "$DST/scripts/$F.new" 2>/dev/null || true
    mv "$DST/scripts/$F.new" "$DST/scripts/$F" || fail "install_$F"
  done

  for F in gen2_keyframes.sh gen2_resync.sh gen2_status.sh; do
    [ -r "$RUNTIME/diagnostics/$F" ] || fail "missing_runtime_$F"
    cp "$RUNTIME/diagnostics/$F" "$DST/scripts/$F.new" || fail "copy_$F"
    chmod 755 "$DST/scripts/$F.new" 2>/dev/null || true
    mv "$DST/scripts/$F.new" "$DST/scripts/$F" || fail "install_$F"
  done

  F=gen2_nav_config.sh
  [ -r "$RUNTIME/navigation/$F" ] || fail "missing_runtime_$F"
  cp "$RUNTIME/navigation/$F" "$DST/scripts/$F.new" || fail "copy_$F"
  chmod 755 "$DST/scripts/$F.new" 2>/dev/null || true
  mv "$DST/scripts/$F.new" "$DST/scripts/$F" || fail "install_$F"

  cp "$RUNTIME/auto-direct/altscreen111.conf" "$DST/config/altscreen111.conf.new" || fail "copy_config"
  chmod 644 "$DST/config/altscreen111.conf.new" 2>/dev/null || true
  mv "$DST/config/altscreen111.conf.new" "$DST/config/altscreen111.conf" || fail "install_config"

  echo "$MEDIA_ROOT" > "$DST/config/deployment_media_root.new" || fail "write_deployment_media_root"
  chmod 644 "$DST/config/deployment_media_root.new" 2>/dev/null || true
  mv "$DST/config/deployment_media_root.new" "$DST/config/deployment_media_root" || fail "install_deployment_media_root"

  app_ro
  echo "STAGE_RUNTIME=PASS"
}

install_required_navignore(){
  if [ ! -r "$PAYLOAD/MIBR-NavIgnore.jar" ]; then
    return 0
  fi

  JARDIR=/mnt/app/eso/hmi/lsd/jars
  JAR=$JARDIR/MIBR-NavIgnore.jar
  TMP=/tmp/lsd.sh.mibr-deploy-navignore.$$
  MARKER='# MIBR NAVIGNORE'
  OWNED=/mnt/app/root/mibr-deploy-navignore-owned

  if grep -Fq 'MIBR-NavIgnore.jar' "$LSD" 2>/dev/null; then
    [ -r "$JAR" ] || fail "navignore_bootclasspath_without_jar"
    H=$(hashf "$JAR") || fail "installed_navignore_hash_failed"
    [ "$H" = "$EXPECTED_NAVIGNORE" ] || fail "installed_navignore_unknown_hash=$H"
    echo "NAVIGNORE=ALREADY_PRESENT"
    return 0
  fi

  app_rw || fail "navignore_mount_app_rw"
  mkdir -p "$JARDIR" || fail "navignore_jardir"
  cp "$PAYLOAD/MIBR-NavIgnore.jar" "$JAR.new" || fail "navignore_copy"
  chmod 644 "$JAR.new" 2>/dev/null || true
  mv "$JAR.new" "$JAR" || fail "navignore_install_jar"

  awk -v marker="$MARKER" '
    BEGIN { done=0 }
    !done && /^\$J9/ {
      print marker
      print "BOOTCLASSPATH=\"$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/MIBR-NavIgnore.jar\""
      done=1
    }
    { print }
    END { if (!done) exit 42 }
  ' "$LSD" > "$TMP" || fail "navignore_patch_lsd"
  chmod 755 "$TMP" 2>/dev/null || true
  mv "$TMP" "$LSD" || fail "navignore_install_lsd"
  : > "$OWNED" || fail "navignore_owned_marker"
  app_ro
  echo "NAVIGNORE=INSTALLED"
}

install_required_most20(){
  JARDIR=/mnt/app/eso/hmi/lsd/jars
  JAR=$JARDIR/MIBR-Most20FPS.jar
  TMP=/tmp/lsd.sh.mibr-deploy-most20.$
  MARKER='# MIBR MOST20FPS'
  OWNED=/mnt/app/root/mibr-deploy-most20-owned

  if grep -Fq 'MIBR-Most20FPS.jar' "$LSD" 2>/dev/null; then
    [ -r "$JAR" ] || fail "most20_bootclasspath_without_jar"
    H=$(hashf "$JAR") || fail "installed_most20_hash_failed"
    [ "$H" = "$EXPECTED_MOST20" ] || fail "installed_most20_unknown_hash=$H"
    echo "MOST20=ALREADY_PRESENT"
    return 0
  fi

  app_rw || fail "most20_mount_app_rw"
  mkdir -p "$JARDIR" || fail "most20_jardir"
  cp "$PAYLOAD/MIBR-Most20FPS.jar" "$JAR.new" || fail "most20_copy"
  chmod 644 "$JAR.new" 2>/dev/null || true
  mv "$JAR.new" "$JAR" || fail "most20_install_jar"

  awk -v marker="$MARKER" '
    BEGIN { done=0 }
    !done && /^\$J9/ {
      print marker
      print "BOOTCLASSPATH=\"$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/MIBR-Most20FPS.jar\""
      done=1
    }
    { print }
    END { if (!done) exit 42 }
  ' "$LSD" > "$TMP" || fail "most20_patch_lsd"
  chmod 755 "$TMP" 2>/dev/null || true
  mv "$TMP" "$LSD" || fail "most20_install_lsd"
  : > "$OWNED" || fail "most20_owned_marker"
  app_ro
  echo "MOST20=INSTALLED"
}

apply(){
  preflight
  PRC=$?
  if [ "$PRC" -eq 30 ]; then
    handle_java_conflict
    JRC=$?
    if [ "$JRC" -ne 0 ]; then
      echo "MIBR_INSTALL=ABORTED java_state_not_changed"
      exit "$JRC"
    fi
    echo "=== re-run preflight after Java normalization ==="
    preflight || fail "post_java_preflight_failed_rc_$?"
  elif [ "$PRC" -ne 0 ]; then
    exit "$PRC"
  fi

  stage_runtime

  echo "=== install DisplayManager isoTX2 gate preload ==="
  MIBR_GATE_LIB="$PAYLOAD/libmibr_isotx2_gate.so" MIBR_SHA256="$SHA" \
    "$RUNTIME/isotx2-gate/install_preload.sh" --apply || fail "gate_preload_rc_$?"

  echo "=== prepare persistent CarPlay Stream111 preload ==="
  "$DST/scripts/patch_carplay.sh" || fail "patch_carplay_rc_$?"

  install_required_navignore
  install_required_most20

  echo "=== enable persistent D2 keyframe recovery default ==="
  app_rw || fail "d2_persistent_mount_app_rw"
  : > /mnt/app/root/mibr-alt111-keyframe-policy.enabled || fail "d2_persistent_marker"
  app_ro
  echo "D2_KEYFRAMES=PERSISTENT_DEFAULT watchdog_ms=1000 min_gap_ms=1000"

  echo "=== set deployment navigation default ==="
  "$DST/scripts/gen2_nav_config.sh" profile map-rich || fail "map_rich_profile"

  echo "=== enable Auto-Direct persistence without pre-reboot runtime takeover ==="
  MIBR_PREPARE_ONLY=1 "$DST/scripts/direct_ts_auto_enable.sh" || fail "auto_direct_prepare_rc_$?"

  echo
  echo "MIBR_INSTALL=PASS"
  echo "navigation_profile=map-rich"
  echo "most20=enabled_by_default"
  echo "d2_keyframes=persistent_default"
  echo "d2_watchdog_ms=1000"
  echo "REBOOT_REQUIRED=YES"
  echo "After reboot: ./status.sh"
  echo "Disable D2 at runtime/persistently with: /mnt/app/root/altscreen-u2/scripts/gen2_keyframes.sh off"
}

case "${1:---check}" in
  --check) preflight ;;
  --apply) apply ;;
  *) echo "Usage: $0 [--check|--apply]"; exit 2 ;;
esac
