#!/usr/bin/env bash
#libsc_build.sh
#Makes system call library for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p out/picolibc/arm-none-eabi/include/
cp -r src/libsc/*.h out/picolibc/arm-none-eabi/include/

