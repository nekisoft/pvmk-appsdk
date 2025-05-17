#!/usr/bin/env bash
#wrappers_build.sh
#Makes compiler wrapper scripts for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -e

mkdir -p out/bin
cp -r src/wrappers/* out/bin/

