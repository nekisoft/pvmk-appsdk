#!/usr/bin/env bash
#libcxx_build.sh
#Makes libcxx for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#set -x

mkdir -p obj/libcxx
pushd obj/libcxx

tar -xf ../../src/libcxx-17.0.1.src.tar.xz
tar -xf ../../src/libcxxabi-17.0.1.src.tar.xz

SITECFG="libcxx-17.0.1.src/include/__config_site"
cat <<EOF > ${SITECFG}
#ifndef __SITECFG_H
#define __SITECFG_H
#define _LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS 1
#define _LIBUNWIND_HAS_NO_THREADS 1
#define _LIBUNWIND_IS_BAREMETAL 1
#define _LIBCPP_STD_VER 20
#define _LIBCPP_HAS_NO_THREADS 1
#define _LIBCPP_PSTL_CPU_BACKEND_SERIAL 1
#define _LIBCPP_ABI_NAMESPACE LIBCPPV1
#define _LIBCPP_ABI_VERSION 1
#define _LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE 1
#define _GNU_SOURCE
#define timespec_get localtime
#ifndef _LIBCPPABI_VERSION 
#define _LIBCPPABI_VERSION 15000
#endif
#define strtof_l(a,b,c)     strtof(a,b)
#define strtod_l(a,b,c)     strtod(a,b)
#define strtold_l(a,b,c)    strtold(a,b)
#define strtoll_l(a,b,c,d)  strtoll(a,b,c)
#define strtoull_l(a,b,c,d) strtoull(a,b,c)
#define wcstoll_l(a,b,c,d)  wcstoll(a,b,c)
#define wcstoull_l(a,b,c,d) scstoull(a,b,c)
#define wcstold_l(a,b,c)    wcstold(a,b)
#endif //__SITECFG_H
EOF

CFLAGS="-nostdlib -nostdinc -ffreestanding "

CFLAGS+="-Ilibcxx-17.0.1.src/include/ "
CFLAGS+="-Ilibcxx-17.0.1.src/src/ "
CFLAGS+="-Ilibcxxabi-17.0.1.src/include/ "
CFLAGS+="-I../libunwind/libunwind-17.0.1.src/include/ "
CFLAGS+="-I../../out/picolibc/arm-none-eabi/include/ "

CFLAGS+="-D_LIBCPP_BUILDING_LIBRARY "
CFLAGS+="-DLIBCXX_BUILDING_LIBCXXABI "
CFLAGS+="-D__rtems__ "

#CFLAGS+="-E -Wp,-v -dM -E "

CXXSRC=$(find libcxx-17.0.1.src/src/ -name *.cpp | sed 's/libcxx-17.0.1.src\/src\///g')
CXXSRC=$(echo ${CXXSRC} | xargs -n1 | grep -v ibm | grep -v libdispatch | grep -v win32)
for CXXFILE in ${CXXSRC}
do
	echo ${CXXFILE}
	mkdir -p $(dirname ${CXXFILE})
	clang++14 -target arm-none-eabi -march=armv5t -std=c++20 ${CFLAGS} libcxx-17.0.1.src/src/${CXXFILE} -c -o ${CXXFILE}.o
done

AR=../../out/bin/pvmk-ar
OUTDIR="../../out/picolibc/arm-none-eabi"
mkdir -p ${OUTDIR}/lib/
${AR} -r ${OUTDIR}/lib/libc++.a *.o

rm -rf ../../out/picolibc/arm-none-eabi/include/c++
cp -r libcxx-17.0.1.src/include/ ../../out/picolibc/arm-none-eabi/include/c++

popd
