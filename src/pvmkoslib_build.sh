#!/usr/bin/env bash
#pvmkoslib_build.sh
#Makes picolibc OS support library for PVMK
#Bryan E. Topp <betopp@betopp.com> 2024


CFLAGS="-std=c99 -Wall -Werror -Wextra -pedantic -Isrc/libsc -O1"
CC="arm-none-eabi-gcc"
AR="arm-none-eabi-ar"

mkdir -p obj/pvmkoslib
${CC} ${CFLAGS} -c src/pvmkoslib/pvmkoslib.c -o obj/pvmkoslib/pvmkoslib.o

mkdir -p out/picolibc/arm-none-eabi/lib/
${AR} -r out/picolibc/arm-none-eabi/lib/libpvmkoslib.a obj/pvmkoslib/pvmkoslib.o




