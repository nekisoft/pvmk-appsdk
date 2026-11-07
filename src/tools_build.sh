#!/usr/bin/env bash
#tools_build.sh
#Installs host tools that ship in the SDK but are not compiled here
#Bryan E. Topp <betopp@betopp.com> 2026

set -e

mkdir -p out/bin
cp src/tools/pvmk-run out/bin/pvmk-run
chmod +x out/bin/pvmk-run
