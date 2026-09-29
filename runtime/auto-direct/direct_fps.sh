#!/bin/ksh
# SPDX-License-Identifier: GPL-3.0-or-later
. /mnt/app/root/altscreen-u2/scripts/common.sh
runtime_init || { echo "DIRECT_FPS=FAIL_RUNTIME"; exit 3; }
load_altscreen_config || { echo "DIRECT_FPS=FAIL_CONFIG"; exit 4; }

FILE=$DIRECT_FPS_OVERRIDE_FILE
BRIDGEPID=/tmp/mibr-direct-auto-bridge.pid

show_status(){
  load_altscreen_config >/dev/null 2>&1 || true
  echo "source_max_fps=$ALTSCREEN111_FPS"
  echo "direct_output_fps=$DIRECT_OUTPUT_FPS"
  echo "direct_pace=$DIRECT_PACE"
  echo "direct_pace_buffer=$DIRECT_PACE_BUFFER"
  [ -r "$FILE" ] && echo "override=$(cat "$FILE" 2>/dev/null)" || echo "override=none"
  if [ -r /tmp/mibr-direct-remux.status ]; then
    grep -E '^(pace_|last_input_interval_us|min_input_interval_us|max_input_interval_us|last_emit_interval_us|max_emit_jitter_us)=' /tmp/mibr-direct-remux.status 2>/dev/null || true
  fi
}

case "${1:-status}" in
  status)
    show_status
    exit 0
    ;;
  20|25|30)
    FPS=$1
    mount -uw /mnt/app 2>/dev/null || { echo "DIRECT_FPS=FAIL_MOUNT_RW"; exit 10; }
    TMP="$FILE.new.$$"
    echo "$FPS" > "$TMP" || {
      rm -f "$TMP" 2>/dev/null || true
      mount -ur /mnt/app 2>/dev/null || true
      echo "DIRECT_FPS=FAIL_WRITE"
      exit 11
    }
    mv "$TMP" "$FILE" || {
      rm -f "$TMP" 2>/dev/null || true
      mount -ur /mnt/app 2>/dev/null || true
      echo "DIRECT_FPS=FAIL_RENAME"
      exit 12
    }
    sync
    mount -ur /mnt/app 2>/dev/null || true
    echo "DIRECT_FPS=SET fps=$FPS"
    if [ "$FPS" != "$ALTSCREEN111_FPS" ]; then
      echo "SOURCE_RATE_NOTE=CarPlay source is still advertised at $ALTSCREEN111_FPS fps in the current session"
      echo "SOURCE_RATE_NOTE=reconnect/re-negotiate CarPlay before judging $FPS fps as a matched source/sink test"
    fi
    P=
    [ -r "$BRIDGEPID" ] && P=$(cat "$BRIDGEPID" 2>/dev/null)
    if [ -n "$P" ] && kill -0 "$P" 2>/dev/null; then
      kill "$P" 2>/dev/null || true
      echo "bridge_restart_requested=1"
    else
      echo "bridge_restart_requested=0"
    fi
    exit 0
    ;;
  default)
    mount -uw /mnt/app 2>/dev/null || { echo "DIRECT_FPS=FAIL_MOUNT_RW"; exit 10; }
    rm -f "$FILE" 2>/dev/null || true
    sync
    mount -ur /mnt/app 2>/dev/null || true
    echo "DIRECT_FPS=DEFAULT"
    P=
    [ -r "$BRIDGEPID" ] && P=$(cat "$BRIDGEPID" 2>/dev/null)
    if [ -n "$P" ] && kill -0 "$P" 2>/dev/null; then
      kill "$P" 2>/dev/null || true
      echo "bridge_restart_requested=1"
    else
      echo "bridge_restart_requested=0"
    fi
    exit 0
    ;;
  *)
    echo "usage: $0 status|20|25|30|default"
    exit 64
    ;;
esac
