#!/usr/bin/env bash
#zlib_build.sh
#Makes zlib for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p obj/zlib
pushd obj/zlib

SRCDIR="../../src/zlib/src"
INCDIR="../../src/zlib/include"

CFLAGS="-I../libunwind/libunwind-17.0.1.src/include/ -I../../out/picolibc/arm-none-eabi/include/ -D_LIBUNWIND_HAS_NO_THREADS -D_LIBUNWIND_IS_BAREMETAL"
CFLAGS+=" -Wno-implicit-function-declaration "

CSRC=${SRCDIR}/*.c
for CFILE in ${CSRC}
do	
	CFILENAME=$(basename ${CFILE})
	echo ${CFILENAME}
	../../out/bin/pvmk-cc -nostdinc ${CFILE} ${CFLAGS} -c -o ${CFILENAME}.o
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/picolibc/arm-none-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libz.a *.o

cp ${INCDIR}/* ${OUTDIR}/include/

popd
