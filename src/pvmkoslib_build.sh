#!/usr/bin/env bash
#pvmkoslib_build.sh
#Makes picolibc OS support library for PVMK
#Bryan E. Topp <betopp@betopp.com> 2024

set -e

CFLAGS="-nostdinc -nostdlib -std=c99 -Wall -Werror -Wextra -pedantic -fno-short-enums -fsigned-char -Isrc/pvmkoslib -Iout/armv5te-pvmk-eabi/include -Isrc/libsc -O1 -g"
CC="arm-none-eabi-gcc"
AR="arm-none-eabi-ar"

OUTDIR="out/armv5te-pvmk-eabi"

#Copy headers that picolibc doesn't provide
mkdir -p ${OUTDIR}/include/
mkdir -p ${OUTDIR}/include/sys
cp src/pvmkoslib/*.h   ${OUTDIR}/include/
cp src/pvmkoslib/sys/*.h ${OUTDIR}/include/sys/

#Compile library objects
mkdir -p obj/pvmkoslib
for CFILE in cdfs pvmkoslib atomics
do
	echo $CFILE
	${CC} ${CFLAGS} -c src/pvmkoslib/${CFILE}.c -o obj/pvmkoslib/${CFILE}.o
	LINKOBJ+=" "
	LINKOBJ+=obj/pvmkoslib/${CFILE}.o
done

#Make library archive in target
mkdir -p ${OUTDIR}/lib/
echo libpvmkoslib.a
${AR} -r ${OUTDIR}/lib/libpvmkoslib.a ${LINKOBJ}







