#!/usr/bin/env python3
"""profan.py - read a nemul instruction histogram and say where the cycles went.

  profan.py <elf> <histogram.txt> [--top N] [--mem CYCLES] [--pcs N]

The emulator counts how many times each program counter executed (NEMUL_PROF). That is an
exact measurement of the instruction mix and nothing else: nemul has no memory system, no
cache and no pipeline, so "cycles" here are a model applied afterwards, in this file, where
the weights can be seen and argued with.

The model is ARMv5TE issue cost with a flat memory penalty:

  data processing                    1
  MUL / MLA / UMULL / SMULL          2
  LDR / LDRB / LDRH                  3
  STR / STRB / STRH                  2
  LDM (n registers)                  2 + n
  STM (n registers)                  2 + n
  B / BL / BX                        3
  SWI                                2
  anything else                      1

--mem adds extra cycles to every load and store, for the SDRAM the real machine has and the
emulator does not. The default is zero, so the numbers are pure issue cost; a Quake surface
is textured by reads scattered over a 64K texture, so on hardware the memory-heavy functions
would come out relatively worse than this predicts.

Function attribution comes from `nm -l -S`, so each count lands on the function that owns
that address and on the source file the compiler says it came from.
"""

import argparse
import bisect
import collections
import struct
import subprocess
import sys

NM = "/usr/local/bin/arm-none-eabi-nm"


def load_symbols(elf):
    """[(addr, size, name, file)] from nm, sorted by address."""
    out = subprocess.run([NM, "-l", "-S", "--defined-only", "--numeric-sort", elf],
                         capture_output=True, text=True, check=True).stdout
    syms = []
    for line in out.splitlines():
        #Lines with no debug line info (hand-written assembly, most of the soft-float
        #library) have no file:line field at all, so the tab is optional.
        parts = line.split("\t")
        head = parts[0].split()
        #nm leaves the size out of the line when the symbol has none, which is the normal
        #case for hand-written assembly - the whole soft-float library, for instance. Those
        #get their extent from the next symbol instead, below.
        if len(head) == 3:
            addr, size, typ, name = head[0], "0", head[1], head[2]
        elif len(head) >= 4:
            addr, size, typ, name = head[0], head[1], head[2], head[3]
        else:
            continue
        if typ not in ("T", "t", "W", "w"):
            continue
        src = parts[1] if len(parts) > 1 else ""
        fname, _, line_no = src.partition(":")
        syms.append([int(addr, 16), int(size, 16), name, fname])

    syms.sort()
    #Fill in a missing size from the next symbol, and make sure no symbol claims an address
    #beyond the next one (aliases share an address; the last name wins, as the linker's do).
    for nn in range(len(syms) - 1):
        here, nxt = syms[nn], syms[nn + 1]
        span = nxt[0] - here[0]
        if here[1] == 0:
            here[1] = span
        elif span > 0 and here[1] > span:
            here[1] = span
    if syms:
        syms[-1][1] = max(syms[-1][1], 4)
    return [tuple(s) for s in syms]


def load_segments(elf):
    """[(vaddr, filesz, offset)] for PT_LOAD, so an address can be turned into file bytes."""
    with open(elf, "rb") as f:
        hdr = f.read(52)
        if hdr[:4] != b"\x7fELF":
            sys.exit(f"{elf}: not an ELF file")
        e_phoff, = struct.unpack_from("<I", hdr, 28)
        e_phentsize, e_phnum = struct.unpack_from("<HH", hdr, 42)
        segs = []
        for n in range(e_phnum):
            f.seek(e_phoff + n * e_phentsize)
            ph = f.read(e_phentsize)
            p_type, p_offset, p_vaddr = struct.unpack_from("<III", ph, 0)
            p_filesz, = struct.unpack_from("<I", ph, 16)
            if p_type == 1 and p_filesz > 0:
                segs.append((p_vaddr, p_filesz, p_offset))
    return segs


class Image:
    def __init__(self, elf):
        self.segs = load_segments(elf)
        with open(elf, "rb") as f:
            self.data = f.read()

    def word(self, vaddr):
        for vaddr0, filesz, offset in self.segs:
            if vaddr0 <= vaddr < vaddr0 + filesz:
                at = offset + (vaddr - vaddr0)
                return struct.unpack_from("<I", self.data, at)[0]
        return None


def cost(insn, mem=0):
    """Issue cost of one ARM instruction, plus mem per memory access."""
    if insn is None:
        return 1
    op = (insn >> 25) & 0x7
    if op == 0b101:                       # B / BL
        return 3
    if (insn >> 24) & 0xF == 0b1111:      # SWI
        return 2
    if (insn >> 26) & 0x3 == 0b01:        # single data transfer
        load = (insn >> 20) & 1
        return (3 if load else 2) + mem
    if (insn >> 25) & 0x7 == 0b011:       # halfword / coprocessor transfers
        if (insn >> 24) & 0xF == 0b1110:  # MCR/MRC
            return 1
        load = (insn >> 20) & 1
        return (3 if load else 2) + mem
    if (insn >> 24) & 0xF == 0b100:       # LDM / STM
        load = (insn >> 20) & 1
        n = bin(insn & 0xFFFF).count("1")
        return 2 + n + (mem if n else 0)
    if (insn >> 26) & 0x3 == 0 and (insn >> 4) & 0xF in (0b1001, 0b1000, 0b1011):
        return 2                           # multiplies and swaps
    return 1                               # data processing, PSR, BX, etc.


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("hist")
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--mem", type=int, default=0)
    ap.add_argument("--pcs", type=int, default=0, help="also list the N hottest instructions")
    ap.add_argument("--calls", type=int, default=0,
                    help="also list the N hottest call sites as caller -> callee")
    ap.add_argument("--file", default=None, help="only this source file's functions")
    args = ap.parse_args()

    syms = load_symbols(args.elf)
    addrs = [s[0] for s in syms]
    image = Image(args.elf)

    def owner(pc):
        at = bisect.bisect_right(addrs, pc) - 1
        if at < 0:
            return ("(no symbol)", "")
        addr, size, name, fname = syms[at]
        if size and pc >= addr + size:
            return (f"(no symbol {pc:08X})", "")
        return (name, fname)

    hist = {}
    meta = {}
    with open(args.hist) as f:
        for line in f:
            if line.startswith("#"):
                k, _, v = line[1:].strip().partition(" ")
                meta[k] = v
                continue
            if not line.strip():
                continue
            a, c = line.split()
            hist[int(a, 16)] = int(c)

    by_func = collections.Counter()
    by_file = collections.Counter()
    cyc_func = collections.Counter()
    cyc_file = collections.Counter()
    pcs = []

    for pc, count in hist.items():
        name, fname = owner(pc)
        w = cost(image.word(pc), args.mem) * count
        by_func[name] += count
        cyc_func[name] += w
        key = fname.split("/")[-1] if fname else "(no debug info)"
        by_file[key] += count
        cyc_file[key] += w
        if args.pcs:
            pcs.append((w, pc, count, name))

    total_i = sum(by_func.values())
    total_c = sum(cyc_func.values())
    if total_i == 0:
        sys.exit("histogram is empty")

    window = f"{meta.get('from','?')}..{meta.get('to','?')} ms"
    print(f"window {window}   instructions {total_i:,}   modelled cycles {total_c:,}"
          f"   ({total_c/total_i:.2f} cycles/instruction)")
    print()

    def table(title, counts, cycles, top):
        print(title)
        print(f"  {'percent':>8} {'cycles':>14} {'instructions':>14}  name")
        for name, cyc in cycles.most_common(top):
            ins = counts[name]
            print(f"  {100.0*cyc/total_c:7.2f}% {cyc:14,} {ins:14,}  {name}")
        print()

    if args.file:
        by_func = collections.Counter({k: v for k, v in by_func.items()})
    table("by function", by_func, cyc_func, args.top)
    table("by source file", by_file, cyc_file, args.top)

    if args.calls:
        #Which call sites run most, and into what. A branch with link is the only way into a
        #library, so counting the BLs attributes the library's time to whoever asked for it -
        #which matters here, because a single division helper is a sixth of the run.
        edges = collections.Counter()
        edge_cost = collections.Counter()
        for pc, count in hist.items():
            insn = image.word(pc)
            if insn is None or (insn >> 25) & 0x7 != 0b101 or not (insn >> 24) & 1:
                continue
            off = insn & 0xFFFFFF
            if off & 0x800000:
                off -= 0x1000000
            target = (pc + 8 + off * 4) & 0xFFFFFFFF
            caller, _ = owner(pc)
            callee, _ = owner(target)
            if callee.startswith("(no") or callee.startswith("(none"):
                callee = f"?{target:08X}"
            edges[(caller, callee)] += count
            edge_cost[(caller, callee)] += cost(insn, args.mem) * count

        print(f"hottest call sites ({args.calls})")
        for (caller, callee), n in edges.most_common(args.calls):
            print(f"  {n:14,} calls  {caller} -> {callee}")
        print()

    if args.pcs:
        print(f"hottest instructions ({args.pcs}, by modelled cycles)")
        for w, pc, count, name in sorted(pcs, reverse=True)[:args.pcs]:
            print(f"  {w:14,} {count:12,}  {pc:08X}  {name}  {image.word(pc):08X}")

    sc = args.hist.rsplit("-", 1)[0] + "-sysc.txt"
    try:
        with open(sc) as f:
            lines = [l.rstrip() for l in f if l.strip()]
    except OSError:
        return
    if lines:
        print()
        print("system calls")
        for line in lines:
            print("  " + line)


if __name__ == "__main__":
    main()
