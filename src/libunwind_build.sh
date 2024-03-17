#!/usr/bin/env bash
#libunwind_build.sh
#Makes libunwind for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p obj/libunwind
pushd obj/libunwind

tar -xf ../../src/libunwind-17.0.1.src.tar.xz

CFLAGS="-I../libunwind/libunwind-17.0.1.src/include/ -I../../out/picolibc/arm-none-eabi/include/ -D_LIBUNWIND_HAS_NO_THREADS -D_LIBUNWIND_IS_BAREMETAL"

CXXSRC="libunwind.cpp Unwind-EHABI.cpp"
for CXXFILE in ${CXXSRC}
do
	echo ${CXXFILE}
	../../out/bin/pvmk-cxx -nostdinc libunwind-17.0.1.src/src/${CXXFILE} ${CFLAGS} -c -o ${CXXFILE}.o
done

CSRC="UnwindLevel1.c UnwindLevel1-gcc-ext.c Unwind-sjlj.c UnwindRegistersSave.S UnwindRegistersRestore.S "
for CFILE in ${CSRC}
do
	echo ${CFILE}
	../../out/bin/pvmk-cc -nostdinc libunwind-17.0.1.src/src/${CFILE} ${CFLAGS} -c -o ${CFILE}.o
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/picolibc/arm-none-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libc++.a *.o

popd
