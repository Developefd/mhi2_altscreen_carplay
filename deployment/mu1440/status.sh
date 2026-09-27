#!/bin/ksh
set -u

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
if [ -x "$DST/scripts/gen2_safearea.sh" ]; then
  "$DST/scripts/gen2_safearea.sh" status
else
  echo "gen2_safearea=missing"
fi

echo
if [ -x "$DST/scripts/verify_carplay111_boot.sh" ]; then
  "$DST/scripts/verify_carplay111_boot.sh"
fi
