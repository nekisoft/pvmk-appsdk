#!/usr/bin/env bash
#pvmkpicocrt_build.sh
#Makes C Runtime for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p out/picolibc/arm-none-eabi/lib/
CRTSRC="crt0 crtbegin crtend "
for CRTFILE in ${CRTSRC}
do
	echo ${CRTFILE}
	arm-none-eabi-as src/pvmkpicocrt/${CRTFILE}.s -o out/picolibc/arm-none-eabi/lib/${CRTFILE}.o
done

cp src/pvmkpicocrt/pvmk.ld out/picolibc/arm-none-eabi/lib/
cp src/pvmkpicocrt/pvmk.specs out/picolibc/arm-none-eabi/lib/
#cp src/pvmkpicocrt/pvmkpp.specs out/picolibc/arm-none-eabi/lib/
cp $(arm-none-eabi-gcc -march=armv5t -print-libgcc-file-name) out/picolibc/arm-none-eabi/lib/

