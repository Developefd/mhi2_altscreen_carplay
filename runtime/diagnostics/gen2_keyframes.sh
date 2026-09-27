#!/bin/ksh
set -u

MARKER=/tmp/mibr-alt111-keyframe-policy.enabled
STATUS=/tmp/mibr-alt111-gen2.status

usage(){
  echo "usage: $0 status|on|off"
  exit 2
}

case "${1:-status}" in
  status)
    [ -e "$MARKER" ] && echo "GEN2_D2_KEYFRAMES=ENABLED" || echo "GEN2_D2_KEYFRAMES=DISABLED"
    if [ -r "$STATUS" ]; then
      grep '^d2_' "$STATUS" 2>/dev/null || true
      grep '^resync_' "$STATUS" 2>/dev/null || true
      grep '^source_idrs=' "$STATUS" 2>/dev/null || true
    else
      echo "GEN2_CORE_STATUS=NOT_AVAILABLE"
    fi
    ;;
  on)
    touch "$MARKER" || exit 1
    sync
    echo "GEN2_D2_KEYFRAMES=ENABLED"
    echo "Policy: turns-edge/suggestUI -> 250ms debounce; 1s IDR watchdog; 1s minimum request gap."
    echo "No reboot required. Stream 111 stays alive."
    ;;
  off)
    rm -f "$MARKER"
    sync
    echo "GEN2_D2_KEYFRAMES=DISABLED"
    echo "No reboot required. Manual Candidate-D controls remain separate."
    ;;
  *)
    usage
    ;;
esac
