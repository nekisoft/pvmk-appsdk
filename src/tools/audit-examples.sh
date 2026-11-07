#!/bin/sh
# audit-examples.sh - try to build every project under src/examples and say which failed
# Usage: audit-examples.sh <sdk-out-dir> [logdir]
set -u
SDK=${1:?usage: audit-examples.sh <sdk-out-dir> [logdir]}
LOG=${2:-/tmp/pvmk-example-audit}
mkdir -p "$LOG"
cd "$(dirname "$0")/../.." || exit 1

MAKE=gmake
command -v gmake >/dev/null 2>&1 || MAKE=make

fail=0
for mk in $(find src/examples -name Makefile -o -name Makefile.dev | sort)
do
	dir=$(dirname "$mk")
	case "$dir" in
		*/orig*) continue ;;
	esac
	name=$(echo "$dir" | sed 's|src/examples/||; s|/|_|g')
	log="$LOG/$name.log"
	printf '%-36s ' "$name"
	if [ "${mk##*/}" = "Makefile.dev" ]
	then
		if ( cd "$dir" && ulimit -t 300 && "$MAKE" -f Makefile.dev ) >"$log" 2>&1
		then echo "ok (dev)"; else echo "FAIL (dev)"; fail=$((fail+1)); fi
	else
		if ( cd "$dir" && ulimit -t 300 && PVMKSDK="$SDK" "$MAKE" ) >"$log" 2>&1
		then echo "ok"; else echo "FAIL"; fail=$((fail+1)); fi
	fi
done
echo "failures: $fail"
exit $((fail > 0))
