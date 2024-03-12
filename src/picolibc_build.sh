#!/usr/bin/env bash
#picolibc_build.sh
#Builds picolibc for PVMK app SDK

pushd obj
tar -xf ../src/picolibc-1.8.6.tar.xz
popd

mkdir obj/picolibc-build
pushd obj/picolibc-build
../picolibc-1.8.6/scripts/do-arm-configure -Dmultilib=false -Dsemihost=false -Dtests=false -Dpicocrt=false -Dpicolib=false -Dposix-console=true -Dprefix=`readlink -f ../../out` -Dspecsdir=`readlink -f ../../out`/picolibc/arm-none-eabi/lib
ninja
ninja install
popd

