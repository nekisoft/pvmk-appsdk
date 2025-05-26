#!/usr/bin/env bash
#binutils_build.sh
#builds GNU BinUtils for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out/bin/$(uname -o)/$(uname -m)/

mkdir -p obj
cd obj
tar -xf ../src/binutils-2.44.tar.xz

mkdir binutils-build
cd binutils-build

OUTDIR=$(readlink -f ../..)/out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)/

../binutils-2.44/configure LDFLAGS="--static" --prefix=${OUTDIR} --target=armv5te-pvmk-eabi --disable-nls --with-sysroot --program-prefix=pvmk- --bindir=${PLATDIR} --libdir=${PLATDIR} --libexecdir=${PLATDIR} --mandir=${OUTDIR}/trash --infodir=${OUTDIR}/trash --docdir=${OUTDIR}/trash --disable-host-shared  --enable-host-static --enable-static --disable-shared 

PVMKMAKE=$(readlink -f ../../out/bin/pvmk-make)

$PVMKMAKE -j
$PVMKMAKE install

