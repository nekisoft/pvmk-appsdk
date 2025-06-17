#!/usr/bin/env bash
#launchers_build.sh
#Makes shell launcher scripts for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out/bin
cp -r src/launchers/* out/

