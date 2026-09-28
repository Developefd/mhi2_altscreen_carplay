#!/bin/ksh
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Read-only compatibility collector for unknown MHI2 targets.
# It does not install hooks or remount /mnt/app or /mnt/system writable.
# The only writable target is the deployment SD/USB medium used for logs.

SELF=$0
ROOT=${SELF%/*}
[ "$ROOT" = "$SELF" ] && ROOT=.
ROOT=$(cd "$ROOT" 2>/dev/null && pwd)
[ -n "$ROOT" ] || { echo "MIBR_COMPAT=FAIL cannot_resolve_root"; exit 20; }

PAYLOAD=$ROOT/payload
SHA=$PAYLOAD/sha256sum
TEE=$PAYLOAD/tee
SESSION_HELPER=$ROOT/runtime/deployment/session.sh

[ -r "$SESSION_HELPER" ] || { echo "MIBR_COMPAT=FAIL missing_session_helper=$SESSION_HELPER"; exit 20; }
. "$SESSION_HELPER" || { echo "MIBR_COMPAT=FAIL cannot_source_session_helper"; exit 20; }

if [ "${MIBR_LOG_ACTIVE:-0}" != "1" ]; then
  [ -x "$SHA" ] || { echo "MIBR_COMPAT=FAIL missing_sha256_helper=$SHA"; exit 20; }
  [ -x "$TEE" ] || { echo "MIBR_COMPAT=FAIL missing_tee_helper=$TEE"; exit 20; }

  mibr_prepare_session compatibility "$ROOT" "$TEE" "$SHA" || {
    echo "MIBR_COMPAT=FAIL cannot_create_sd_session rc=$?"
    exit 20
  }

  echo "Compatibility report will be written to $MIBR_SESSION_DIR"
  mibr_run_logged "$0" "$@"
  exit $?
fi

safe_copy(){
  SRC=$1
  DST=$2
  if [ -r "$SRC" ]; then
    cp "$SRC" "$DST" 2>/dev/null && {
      echo "copied $SRC -> $DST"
      return 0
    }
    echo "copy_failed $SRC"
    return 1
  fi
  echo "missing $SRC"
  return 0
}

hash_line(){
  P=$1
  [ -n "$P" ] || return 0
  if [ -r "$P" ]; then
    H=$(mibr_hash_file "$P" 2>/dev/null)
    S=$(wc -c < "$P" 2>/dev/null)
    echo "sha256 ${H:-HASH_FAILED} size=${S:-UNKNOWN} path=$P"
  else
    echo "missing $P"
  fi
}

find_first(){
  for P in "$@"; do
    [ -r "$P" ] && { echo "$P"; return 0; }
  done
  return 1
}

echo "=== MHI2 AltScreen compatibility collector ==="
echo "mode=READ_ONLY_TARGET"
echo "writes=deployment_media_only"
echo "reference_target=MHI2_ER_SKG13_P4526_MU1440"
echo "reference_cluster=AID10-class"

mibr_vehicle_summary || echo "WARN vehicle_summary_failed"

OUTDIR="$MIBR_SESSION_DIR/compatibility"
CFGDIR="$OUTDIR/stock-configs"
mkdir -p "$CFGDIR" 2>/dev/null || {
  echo "MIBR_COMPAT=FAIL cannot_create_compatibility_dir"
  exit 21
}

DISPLAYCFG=$(find_first   /etc/eso/production/displaymanager.json   /mnt/system/etc/eso/production/displaymanager.json   /mnt/app/eso/production/displaymanager.json 2>/dev/null)

DIOCFG=$(find_first   /mnt/system/etc/eso/production/dio_manager.json   /etc/eso/production/dio_manager.json   /mnt/app/eso/production/dio_manager.json 2>/dev/null)

SMARTCFG=$(find_first   /mnt/system/etc/eso/production/smartphone_integrator.json   /etc/eso/production/smartphone_integrator.json   /mnt/app/eso/production/smartphone_integrator.json 2>/dev/null)

echo
echo "=== stock configuration snapshots ==="
[ -n "$DISPLAYCFG" ] && safe_copy "$DISPLAYCFG" "$CFGDIR/displaymanager.json"
[ -n "$DIOCFG" ] && safe_copy "$DIOCFG" "$CFGDIR/dio_manager.json"
[ -n "$SMARTCFG" ] && safe_copy "$SMARTCFG" "$CFGDIR/smartphone_integrator.json"

HASHES="$OUTDIR/component-hashes.txt"
{
  echo "=== MHI2 AltScreen compatibility component hashes ==="
  echo "reference_target=MHI2_ER_SKG13_P4526_MU1440"
  echo "reference_cluster=AID10-class"

  LSDJXE=$(find_first /ifs/lsd.jxe /mnt/app/eso/hmi/lsd/lsd.jxe 2>/dev/null)

  hash_line "$LSDJXE"
  hash_line /mnt/app/eso/hmi/lsd/lsd.sh
  hash_line "$DISPLAYCFG"
  hash_line "$DIOCFG"
  hash_line "$SMARTCFG"
  hash_line /mnt/app/eso/lib/libairplay.so
  hash_line /mnt/app/eso/lib/factories/libdsicarplayproxy.so
  hash_line /mnt/app/armle/usr/sbin/mm-ipod
  hash_line /mnt/app/armle/usr/lib/libiap2client.so.1
  hash_line /mnt/app/eso/bin/apps/smartphone_integrator
  hash_line /mnt/app/eso/bin/apps/dio_manager
  hash_line /mnt/app/eso/bin/apps/displaymanager
} > "$HASHES"

RUNTIME="$OUTDIR/runtime-observation.txt"
{
  echo "=== read-only runtime observations ==="
  echo "--- relevant processes ---"
  pidin ar 2>/dev/null | awk '
    /displaymanager|dio_manager|smartphone_integrator|mm-ipod|lsd/ { print }
  ' || true

  echo "--- MOST / display nodes ---"
  for P in     /dev/mlb/isoTX1     /dev/mlb/isoTX2     /net/rcc/dev/name/local/inic/isoTX1     /net/rcc/dev/name/local/inic/isoTX2; do
    if [ -e "$P" ]; then
      ls -l "$P" 2>/dev/null || echo "present $P"
    else
      echo "missing $P"
    fi
  done

  echo "--- displaymanager config markers ---"
  if [ -n "$DISPLAYCFG" ] && [ -r "$DISPLAYCFG" ]; then
    awk '
      /isoTX|lvds|LVDS|HDMI|force_kombi_type|display|terminal|encoder|MOST/ {
        print NR ":" $0
      }' "$DISPLAYCFG" 2>/dev/null
  fi

  echo "--- dio_manager config markers ---"
  if [ -n "$DIOCFG" ] && [ -r "$DIOCFG" ]; then
    awk '
      /CarPlay|carplay|iAP2|iap2|USB|usb|MOST|mlb|video|display/ {
        print NR ":" $0
      }' "$DIOCFG" 2>/dev/null
  fi

  echo "--- smartphone_integrator config markers ---"
  if [ -n "$SMARTCFG" ] && [ -r "$SMARTCFG" ]; then
    awk '
      /airplay|AirPlay|CarPlay|carplay|preload|library|libairplay|video|screen/ {
        print NR ":" $0
      }' "$SMARTCFG" 2>/dev/null
  fi
} > "$RUNTIME"

MANIFEST="$OUTDIR/README.txt"
{
  echo "MHI2 AltScreen read-only compatibility report"
  echo
  echo "Generated without target-side installation or persistent target mutation."
  echo "Only the deployment SD/USB medium was made writable for report output."
  echo
  echo "Issue-safe defaults:"
  echo "  ../session.log"
  echo "  ../vehicle-summary.txt"
  echo "  component-hashes.txt"
  echo "  runtime-observation.txt"
  echo
  echo "Raw stock config snapshots:"
  echo "  stock-configs/displaymanager.json"
  echo "  stock-configs/dio_manager.json"
  echo "  stock-configs/smartphone_integrator.json"
  echo
  echo "Review raw stock configs before posting publicly."
  echo "VIN, FAZIT and device serial numbers are intentionally not queried by this collector."
} > "$MANIFEST"

echo
cat "$HASHES"
echo
cat "$RUNTIME"
echo
echo "MIBR_COMPAT=PASS_READ_ONLY"
echo "compatibility_dir=$OUTDIR"
echo "vehicle_summary=$MIBR_SESSION_DIR/vehicle-summary.txt"
echo "component_hashes=$HASHES"
echo "runtime_observation=$RUNTIME"
echo "raw_stock_configs=$CFGDIR"
exit 0
