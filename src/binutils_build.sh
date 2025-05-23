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
../binutils-2.44/configure --prefix=$(readlink -f ../..)/out/ --target=armv5te-pvmk-eabi --disable-nls --with-sysroot --program-prefix=pvmk- --bindir=$(readlink -f ../..)/out/bin/$(uname -o)/$(uname -m)/

PVMKMAKE=$(readlink -f ../../out/bin/pvmk-make)

$PVMKMAKE -j
$PVMKMAKE install

mv ../../out/armv5te-pvmk-eabi/bin/* ../../out/bin/$(uname -o)/$(uname -m)/
rm -rf ../../out/armv5te-pvmk-eabi/bin

