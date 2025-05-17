#!/usr/bin/env bash
#libsc_build.sh
#Makes system call library for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -e

mkdir -p out/armv5te-pvmk-eabi/include/
cp -r src/libsc/*.h out/armv5te-pvmk-eabi/include/

