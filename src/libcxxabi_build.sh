#!/usr/bin/env bash
#libcxxabi_build.sh
#Makes libcxxabi for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

mkdir -p obj/libcxxabi
pushd obj/libcxxabi

tar -xf ../../src/libcxxabi-17.0.1.src.tar.xz

CFLAGS="-nostdlib -nostdinc -ffreestanding "
CFLAGS+="-I../libcxx/libcxx-17.0.1.src/include/ "
CFLAGS+="-I../libcxx/libcxx-17.0.1.src/src/ "
CFLAGS+="-I../libcxxabi/libcxxabi-17.0.1.src/include/ "
CFLAGS+="-I../libunwind/libunwind-17.0.1.src/include/ "
CFLAGS+="-I../../out/picolibc/arm-none-eabi/include/ "
CFLAGS+="-Dtimespec_get=localtime "
CFLAGS+="-D__rtems__ "
CFLAGS+="-D_LIBCXXABI_HAS_NO_THREADS "
CFLAGS+="-D_LIBCPP_BUILDING_LIBRARY "

#CFLAGS+="-E -Wp,-v -dM -E "

CXXSRC=""
CXXSRC+="cxa_aux_runtime.cpp "
CXXSRC+="cxa_default_handlers.cpp "
CXXSRC+="cxa_demangle.cpp "
CXXSRC+="cxa_exception.cpp "
CXXSRC+="cxa_guard.cpp "
CXXSRC+="cxa_handlers.cpp "
CXXSRC+="cxa_handlers.cpp "
CXXSRC+="cxa_personality.cpp "
CXXSRC+="cxa_vector.cpp "
CXXSRC+="cxa_virtual.cpp "
CXXSRC+="stdlib_exception.cpp "
CXXSRC+="stdlib_stdexcept.cpp "
CXXSRC+="stdlib_typeinfo.cpp "
CXXSRC+="abort_message.cpp "
CXXSRC+="fallback_malloc.cpp "
CXXSRC+="private_typeinfo.cpp "

for CXXFILE in ${CXXSRC}
do
	echo ${CXXFILE}
	mkdir -p $(dirname ${CXXFILE})
	clang++14 -target arm-none-eabi -std=c++20 ${CFLAGS} libcxxabi-17.0.1.src/src/${CXXFILE} -c -o ${CXXFILE}.o
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/picolibc/arm-none-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libc++.a *.o

popd
