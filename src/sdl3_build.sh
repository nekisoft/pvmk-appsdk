#!/usr/bin/env bash
#sdl3_build.sh
#Makes SDL3 for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -x

mkdir -p obj/sdl3
pushd obj/sdl3

SRCDIR="../../src/sdl3/src"
INCDIR="../../src/sdl3/include"

CFLAGS="-I../libunwind/libunwind-17.0.1.src/include/ -I../../out/armv5te-pvmk-eabi/include/ -D_LIBUNWIND_HAS_NO_THREADS -D_LIBUNWIND_IS_BAREMETAL"
CFLAGS+=" -Wno-implicit-function-declaration "
CFLAGS+=" -I${SRCDIR}"
CFLAGS+=" -I${INCDIR}"
CFLAGS+=" -I${INCDIR}/SDL3"
CFLAGS+=" -I${INCDIR}/SDL3/build_config"
CFLAGS+=" -g -O2"
CFLAGS+=" -Werror"

CSRC=`find ${SRCDIR} -name \*.c`
COBJ=
for CFILE in ${CSRC}
do	
	CFILENAME=$(basename ${CFILE})
	echo ${CFILENAME}
	OBJFILE=./$(readlink -f ${CFILE}).o
	if [ ! -f ${OBJFILE} ]
	then
		mkdir -p $(dirname ${OBJFILE})
		../../out/bin/pvmk-cc -nostdinc ${CFILE} ${CFLAGS} -c -o ${OBJFILE} || exit -1
	fi
	COBJ+=" "
	COBJ+=${OBJFILE}
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/armv5te-pvmk-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libsdl3.a ${COBJ}

cp -r ${INCDIR}/* ${OUTDIR}/include/

popd
