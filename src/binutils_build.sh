#!/usr/bin/env bash
#binutils_build.sh
#builds GNU BinUtils for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p obj
cd obj
tar -xf ../src/binutils-2.44.tar.xz

mkdir binutils-build
cd binutils-build
../binutils-2.44/configure --prefix=$(readlink -f ../..)/out/ --target=armv5te-none-eabi --disable-nls --with-sysroot --program-prefix=pvmk-

gmake -j

gmake install
