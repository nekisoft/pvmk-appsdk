#!/usr/bin/env python3
#nemul_profile.py - per-function cost report from nemul's timing model.
#
#This is the piece that makes the model useful to a developer: run the game under the
#timing build with the histogram switched on, then read the result as "where did the
#frame go" rather than "how many cycles did it take".
#
#	NEMUL_TIMING_HIST_FILE=/tmp/prof.txt nemul-headless --disk ... --nne ... --frames N
#	nemul_profile.py /tmp/prof.txt game.elf
#
#The addresses in the histogram are the addresses the program ran at, so the ELF given
#here must be the one that was loaded.

import argparse
import bisect
import os
import subprocess
import sys


def load_symbols(elf):
    """(start, size, name) for every function, sorted by address."""
    out = subprocess.run(["pvmk-nm", "--print-size", "--defined-only", elf],
                         capture_output=True, text=True)
    if out.returncode != 0:
        #Fall back to whatever nm is on the path.
        out = subprocess.run(["arm-none-eabi-nm", "--print-size", "--defined-only", elf],
                             capture_output=True, text=True)
    syms = []
    for line in out.stdout.splitlines():
        parts = line.split()
        if len(parts) != 4:
            continue
        addr, size, kind, name = parts
        if kind.lower() not in ("t", "w"):
            continue
        try:
            syms.append((int(addr, 16), int(size, 16), name))
        except ValueError:
            continue
    syms.sort()
    return syms


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("hist")
    ap.add_argument("elf")
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--frames", type=int, default=0,
                    help="frames the run covered, to report cycles per frame")
    args = ap.parse_args()

    syms = load_symbols(args.elf)
    starts = [s[0] for s in syms]

    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        if i < 0:
            return "?"
        start, size, name = syms[i]
        if size and addr >= start + size:
            return "? (between functions)"
        return name

    per = {}
    total = 0
    with open(args.hist) as f:
        for line in f:
            parts = line.split()
            if len(parts) != 3:
                continue
            pc = int(parts[0], 16)
            cyc = int(parts[1])
            cnt = int(parts[2])
            name = owner(pc)
            rec = per.setdefault(name, [0, 0])
            rec[0] += cyc
            rec[1] += cnt
            total += cyc

    if total == 0:
        print("empty histogram")
        return 1

    rows = sorted(per.items(), key=lambda kv: -kv[1][0])
    print("%-32s %12s %7s %10s %8s" % ("function", "cycles", "%", "instructions", "c/insn"))
    print("-" * 74)
    for name, (cyc, cnt) in rows[:args.top]:
        print("%-32s %12d %6.2f%% %10d %8.2f" %
              (name, cyc, 100.0 * cyc / total, cnt, cyc / cnt if cnt else 0.0))
    print("-" * 74)
    print("%-32s %12d" % ("total", total))
    if args.frames:
        print("%-32s %12.0f  (%.2f ms per frame at 600MHz)" %
              ("per frame", total / args.frames, total / args.frames / 600000.0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
