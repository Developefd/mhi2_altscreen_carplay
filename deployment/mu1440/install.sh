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
DST=/mnt/app/root/altscreen-u2
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
APP_RW=0

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
EXPECTED_GATE=0b3cf76ab6bbcc12223ca00fa01229a7fe3943ad4d1566fb769181aadf6a57b1
EXPECTED_NAVIGNORE=b065bab0e1c58f8439a3bdd73d2d4cb6060cbac1c943e5b425425eb453c94b34

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

preflight(){
  echo "=== MHI2 AltScreen MU1440 developer install preflight ==="
  [ -x "$SHA" ] || fail "missing_sha256_helper=$SHA"
  require_cmds mount cp mv chmod sync mkdir rm touch sleep grep awk sed wc cat pidin on slay
  [ -r /mnt/app/eso/lib/libairplay.so ] || fail "not_mmx_target"
  AIR=$(hashf /mnt/app/eso/lib/libairplay.so) || fail "libairplay_hash_failed"
  [ "$AIR" = "$EXPECTED_AIRPLAY" ] || fail "unsupported_libairplay=$AIR"
  echo "PASS libairplay=$AIR"

  check_hash "$PAYLOAD/libaltscreen111.so" "$EXPECTED_GEN2"
  check_hash "$PAYLOAD/direct-ts-remux" "$EXPECTED_REMUX"
  check_hash "$PAYLOAD/libmibr_isotx2_gate.so" "$EXPECTED_GATE"

  [ -r "$RUNTIME/auto-direct/common.sh" ] || fail "runtime_common_missing"
  [ -r "$RUNTIME/auto-direct/patch_carplay.sh" ] || fail "patch_carplay_missing"
  [ -r "$RUNTIME/auto-direct/restore_stock.sh" ] || fail "restore_stock_missing"
  [ -r "$RUNTIME/isotx2-gate/install_preload.sh" ] || fail "gate_installer_missing"
  [ -r "$RUNTIME/isotx2-gate/restore_preload.sh" ] || fail "gate_restore_missing"

  if grep -Fq 'MIBR-DirectVCPolicy.jar' "$LSD" 2>/dev/null ||
     grep -Fq '# MIBR DIRECT-VC POLICY' "$LSD" 2>/dev/null; then
    fail "directvc_java_override_present_restore_stock_java_first"
  fi
  if grep -Fq 'Most20FPS.jar' "$LSD" 2>/dev/null; then
    fail "most20_java_override_present_remove_before_install"
  fi
  if grep -Fq 'NavActiveIgnore.jar' "$LSD" 2>/dev/null; then
    fail "legacy_combined_navactiveignore_present_use_split_navignore_only"
  fi

  echo "=== DisplayManager gate preflight ==="
  MIBR_GATE_LIB="$PAYLOAD/libmibr_isotx2_gate.so" MIBR_SHA256="$SHA" \
    "$RUNTIME/isotx2-gate/install_preload.sh" --check || fail "gate_preflight_rc_$?"

  if [ -r "$PAYLOAD/MIBR-NavIgnore.jar" ]; then
    N=$(hashf "$PAYLOAD/MIBR-NavIgnore.jar") || fail "navignore_hash_failed"
    [ "$N" = "$EXPECTED_NAVIGNORE" ] || fail "navignore_hash_mismatch=$N"
    echo "INFO navignore_payload=valid"
  elif grep -Fq 'MIBR-NavIgnore.jar' "$LSD" 2>/dev/null; then
    echo "INFO navignore=already_installed"
  else
    echo "WARN navignore=absent"
    echo "WARN the full vehicle-proven smartphone-navigation VC presentation used NavIgnore"
  fi

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

  app_ro
  echo "STAGE_RUNTIME=PASS"
}

install_optional_navignore(){
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

apply(){
  preflight
  stage_runtime

  echo "=== install DisplayManager isoTX2 gate preload ==="
  MIBR_GATE_LIB="$PAYLOAD/libmibr_isotx2_gate.so" MIBR_SHA256="$SHA" \
    "$RUNTIME/isotx2-gate/install_preload.sh" --apply || fail "gate_preload_rc_$?"

  echo "=== prepare persistent CarPlay Stream111 preload ==="
  "$DST/scripts/patch_carplay.sh" || fail "patch_carplay_rc_$?"

  install_optional_navignore

  echo "=== set deployment navigation default ==="
  "$DST/scripts/gen2_nav_config.sh" profile map-rich || fail "map_rich_profile"

  echo "=== enable Auto-Direct persistence without pre-reboot runtime takeover ==="
  MIBR_PREPARE_ONLY=1 "$DST/scripts/direct_ts_auto_enable.sh" || fail "auto_direct_prepare_rc_$?"

  echo
  echo "MIBR_INSTALL=PASS"
  echo "navigation_profile=map-rich"
  echo "REBOOT_REQUIRED=YES"
  echo "After reboot: ./status.sh"
  echo "Current D2 keyframe policy remains an explicit developer control until its watchdog tuning is finalized."
  echo "For current vehicle behavior after reboot: /mnt/app/root/altscreen-u2/scripts/gen2_keyframes.sh on"
}

case "${1:---check}" in
  --check) preflight ;;
  --apply) apply ;;
  *) echo "Usage: $0 [--check|--apply]"; exit 2 ;;
esac
