#!/usr/bin/env bash
#gcc_build.sh
#builds GNU GCC for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out
mkdir -p obj
cd obj
tar -xf ../src/gcc-15.1.0.tar.xz

mkdir gcc-build
cd gcc-build

OUTDIR=$(readlink -f ../../out)
PVMKMAKE=${OUTDIR}/bin/pvmk-make

../gcc-15.1.0/configure --prefix=${OUTDIR} --target=armv5te-pvmk-eabi --with-native-system-header-dir=${OUTDIR}/armv5te-pvmk-eabi/include --disable-shared --disable-multiarch --disable-multilib --with-endian=little --disable-threads --disable-tls --with-cpu=arm926ej-s --with-mode=arm --disable-libssp --disable-nls --program-prefix=pvmk- --enable-languages=c,c++ --disable-hosted-libstdcxx --without-headers
mkdir -p  ${OUTDIR}/armv5te-pvmk-eabi/include

$PVMKMAKE -j
$PVMKMAKE install


