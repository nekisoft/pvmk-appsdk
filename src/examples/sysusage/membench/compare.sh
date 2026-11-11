#!/bin/sh
#compare.sh - run each shared workload under nemul's timing model and report the
#modelled cycles, for comparing against the board's numbers.
#
#Each build runs one workload (WORKLOAD=0 does nothing). The difference in modelled
#cycles between a workload build and the baseline is what the workload cost, with
#kernel boot cancelling out.
set -e

SDK=${PVMKSDK:-/home/agents/work/pvmk-appsdk/out}
NEMUL=/home/agents/work/pvmk-appsdk/src/nemul/bin/timing/nemul-headless.elf
FRAMES=${FRAMES:-40}

cd "$(dirname "$0")"

for w in 0 1 2 3 4 5 6 7 8 9; do
	PVMKSDK=$SDK gmake -s clean >/dev/null 2>&1 || true
	PVMKSDK=$SDK gmake -s WORKLOAD=$w >/dev/null 2>&1
	out=$($NEMUL --disk img/game.iso --nne bin/boot.nne --frames "$FRAMES" 2>/dev/null)
	cyc=$(echo "$out" | sed -n 's/^model: \([0-9]*\) cycles.*/\1/p')
	ck=$(echo "$out" | sed -n 's/^CHECK \([0-9A-F]*\).*/\1/p')
	echo "$w $cyc $ck"
done
