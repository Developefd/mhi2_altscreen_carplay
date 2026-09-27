#!/usr/bin/env python3
"""Parse stock MU1440 keypanel trace lines.

Expected stock logger line:
  HK Received: KBD[4] KEY[50] KST[1]

This tool runs off-unit. It does not modify the MHI2.
"""
from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

RX = re.compile(r"HK Received:\s*KBD\[(\d+)\]\s*KEY\[(\d+)\]\s*KST\[(\d+)\]")

KBD = {
    4: "KBD_MFW",
}

KEY = {
    35: "KEY_MFW_MENU",
    36: "KEY_MFW_ARROW_RIGHT",
    37: "KEY_MFW_ARROW_LEFT",
    38: "KEY_MFW_UP",
    39: "KEY_MFW_DOWN",
    40: "KEY_MFW_ROLLER_LEFT",
    41: "KEY_MFW_CANCEL",
    42: "KEY_MFW_VOLUME_UP",
    43: "KEY_MFW_VOLUME_DOWN",
    44: "KEY_MFW_ROLLER_RIGHT",
    45: "KEY_MFW_AUDIOSOURCE",
    46: "KEY_MFW_ARROW_A_UP",
    47: "KEY_MFW_ARROW_A_DOWN",
    48: "KEY_MFW_ARROW_B_UP",
    49: "KEY_MFW_ARROW_B_DOWN",
    50: "KEY_MFW_PTT_ON",
    51: "KEY_MFW_PTT_CANCEL",
    52: "KEY_MFW_INFO",
    53: "KEY_MFW_HOOK",
    54: "KEY_MFW_HANGUP",
    55: "KEY_MFW_OFFHOOK",
    56: "KEY_MFW_LIGHT",
    57: "KEY_MFW_MUTE",
    58: "KEY_MFW_JOKER1",
    59: "KEY_MFW_JOKER2",
    60: "KEY_MFW_INIT",
    99: "KEY_MFW_SIDEMENULEFT",
    100: "KEY_MFW_SIDEMENURIGHT",
}

KST = {
    0: "KST_RELEASED",
    1: "KST_PRESSED",
    2: "KST_DOUBLEPRESSED",
    3: "KST_LONGPRESSED",
    4: "KST_LONGPRESSED2",
    5: "KST_LONGPRESSED3",
}


def lines_for(path: str | None):
    if path in (None, "-"):
        yield from sys.stdin
        return
    with Path(path).open("r", errors="replace") as f:
        yield from f


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("trace", nargs="?", default="-", help="trace file or - for stdin")
    ap.add_argument("--csv", action="store_true", help="emit CSV")
    args = ap.parse_args()

    rows = []
    for line_no, line in enumerate(lines_for(args.trace), 1):
        m = RX.search(line)
        if not m:
            continue
        kbd, key, kst = map(int, m.groups())
        rows.append(
            {
                "line": line_no,
                "kbd": kbd,
                "kbd_name": KBD.get(kbd, "KBD_UNKNOWN"),
                "key": key,
                "key_name": KEY.get(key, "KEY_UNKNOWN"),
                "kst": kst,
                "state_name": KST.get(kst, "KST_UNKNOWN"),
            }
        )

    if args.csv:
        w = csv.DictWriter(
            sys.stdout,
            fieldnames=["line", "kbd", "kbd_name", "key", "key_name", "kst", "state_name"],
        )
        w.writeheader()
        w.writerows(rows)
    else:
        for r in rows:
            print(
                f"{r['line']:6d} "
                f"KBD={r['kbd']:3d} {r['kbd_name']:<12} "
                f"KEY={r['key']:3d} {r['key_name']:<24} "
                f"KST={r['kst']:2d} {r['state_name']}"
            )

    if not rows:
        print("No 'HK Received: KBD[...] KEY[...] KST[...]' lines found.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
