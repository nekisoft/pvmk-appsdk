#!/usr/bin/env bash
#pvmkpicocrt_build.sh
#Makes C Runtime for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -e

mkdir -p out/armv5te-pvmk-eabi/lib/
CRTSRC="crt0 crtbegin crtend "
for CRTFILE in ${CRTSRC}
do
	echo ${CRTFILE}
	arm-none-eabi-as src/pvmkpicocrt/${CRTFILE}.s -o out/armv5te-pvmk-eabi/lib/${CRTFILE}.o
done

cp src/pvmkpicocrt/pvmk.ld out/armv5te-pvmk-eabi/lib/
cp src/pvmkpicocrt/pvmk.specs out/armv5te-pvmk-eabi/lib/
#cp src/pvmkpicocrt/pvmkpp.specs out/armv5te-pvmk-eabi/lib/
cp $(arm-none-eabi-gcc -march=armv5t -print-libgcc-file-name) out/armv5te-pvmk-eabi/lib/

