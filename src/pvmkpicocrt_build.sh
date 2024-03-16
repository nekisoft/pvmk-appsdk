#!/usr/bin/env bash
#pvmkpicocrt_build.sh
#Makes C Runtime for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p out/picolibc/arm-none-eabi/lib/
arm-none-eabi-as src/pvmkpicocrt/crt0.s -o out/picolibc/arm-none-eabi/lib/crt0.o
cp src/pvmkpicocrt/pvmk.ld out/picolibc/arm-none-eabi/lib/
cp src/pvmkpicocrt/pvmk.specs out/picolibc/arm-none-eabi/lib/
cp src/pvmkpicocrt/pvmkpp.specs out/picolibc/arm-none-eabi/lib/
