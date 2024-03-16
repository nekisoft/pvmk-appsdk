#!/usr/bin/env bash
#pvmkoslib_build.sh
#Makes picolibc OS support library for PVMK
#Bryan E. Topp <betopp@betopp.com> 2024


CFLAGS="-nostdinc -nostdlib -std=c99 -Wall -Werror -Wextra -pedantic -Iout/picolibc/arm-none-eabi/include -Isrc/libsc -O1"
CC="arm-none-eabi-gcc"
AR="arm-none-eabi-ar"

OUTDIR="out/picolibc/arm-none-eabi"

#Copy headers that picolibc doesn't provide
mkdir -p ${OUTDIR}/include/
mkdir -p ${OUTDIR}/include/sys
cp src/pvmkoslib/*.h   ${OUTDIR}/include/
cp src/pvmkoslib/sys/*.h ${OUTDIR}/include/sys/

#Compile library objects
mkdir -p obj/pvmkoslib
${CC} ${CFLAGS} -c src/pvmkoslib/pvmkoslib.c -o obj/pvmkoslib/pvmkoslib.o

#Make library archive in target
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libpvmkoslib.a obj/pvmkoslib/pvmkoslib.o







