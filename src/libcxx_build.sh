#!/usr/bin/env bash
#libunwind_build.sh
#Makes libunwind for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#set -x

mkdir -p obj/libcxx
pushd obj/libcxx

tar -xf ../../src/libcxx-17.0.1.src.tar.xz

SITECFG="libcxx-17.0.1.src/include/__config_site"
touch ${SITECFG}
echo "#define _EXAMPLE 1" >> ${SITECFG}

CFLAGS="-nostdlib -nostdinc -ffreestanding "

CFLAGS+="-Ilibcxx-17.0.1.src/include/ "
CFLAGS+="-Ilibcxx-17.0.1.src/src/ "
CFLAGS+="-I../libunwind/libunwind-17.0.1.src/include/ "
CFLAGS+="-I../../out/picolibc/arm-none-eabi/include/ "

CFLAGS+="-D_LIBUNWIND_HAS_NO_THREADS -D_LIBUNWIND_IS_BAREMETAL "
CFLAGS+="-D_LIBCPP_PSTL_CPU_BACKEND_SERIAL -D_LIBCPP_HAS_NO_THREADS "
CFLAGS+="-D_LIBCPP_ABI_VERSION=1 -D_LIBCPP_ABI_NAMESPACE=LIBCPPV1 -D_LIBCPP_STD_VER=20 "
CFLAGS+="-Dtimespec_get=localtime "
CFLAGS+="-D_LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS "
CFLAGS+="-D_GNU_SOURCE=1 "
CFLAGS+="-D_LIBCPP_BUILDING_LIBRARY "
CFLAGS+="-D__rtems__ "

#CFLAGS+="-E -Wp,-v -dM -E "

CXXSRC=$(find libcxx-17.0.1.src/src/ -name *.cpp | sed 's/libcxx-17.0.1.src\/src\///g')
CXXSRC=$(echo ${CXXSRC} | xargs -n1 | grep -v ibm | grep -v libdispatch | grep -v win32)
for CXXFILE in ${CXXSRC}
do
	echo ${CXXFILE}
	mkdir -p $(dirname ${CXXFILE})
	clang++14 -target arm-none-eabi -std=c++20 ${CFLAGS} libcxx-17.0.1.src/src/${CXXFILE} -c -o ${CXXFILE}.o
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/picolibc/arm-none-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libcxx.a *.o

popd
