#!/bin/ksh
# Run from project-owned SD folder; no auto-start or OEM mutation.
set -u
[ -x ./mibr-keypanel-native ] || { echo 'ERROR: mibr-keypanel-native not executable'; exit 2; }
DURATION=${1:-120}
exec ./mibr-keypanel-native --capture --out . --seconds "$DURATION"