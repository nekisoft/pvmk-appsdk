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
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

../gcc-15.1.0/configure LDFLAGS="--static" --prefix=${OUTDIR} --target=armv5te-pvmk-eabi --with-native-system-header-dir=${OUTDIR}/armv5te-pvmk-eabi/include --disable-shared --disable-multiarch --disable-multilib --with-endian=little --disable-threads --disable-tls --with-cpu=arm926ej-s --with-mode=arm --disable-libssp --disable-nls --program-prefix=pvmk- --bindir=${PLATDIR} --libdir=${PLATDIR} --libexecdir=${PLATDIR} --enable-languages=c,c++ --disable-hosted-libstdcxx --without-headers --disable-host-shared --infodir=${OUTDIR}/trash --mandir=${OUTDIR}/trash --docdir=${OUTDIR}/trash --enable-host-static --enable-static --disable-shared --disable-lto

mkdir -p  ${OUTDIR}/armv5te-pvmk-eabi/include

$PVMKMAKE -j
$PVMKMAKE install

mv ../../out/armv5te-pvmk-eabi/bin/* ../../out/bin/$(uname -o)/$(uname -m)/
rm -rf ../../out/armv5te-pvmk-eabi/bin

