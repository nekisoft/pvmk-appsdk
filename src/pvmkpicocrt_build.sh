#!/usr/bin/env bash
#pvmkpicocrt_build.sh
#Makes C Runtime for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p out/picolibc/arm-none-eabi/lib/
arm-none-eabi-as src/pvmkpicocrt/crt0.s -o out/picolibc/arm-none-eabi/lib/crt0.o

