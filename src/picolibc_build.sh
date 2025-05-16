#!/usr/bin/env bash
#picolibc_build.sh
#Builds picolibc for PVMK app SDK

pushd obj
tar -xf ../src/picolibc-1.8.6.tar.xz
popd

mkdir obj/picolibc-build
pushd obj/picolibc-build
../picolibc-1.8.6/scripts/do-arm-configure -Dc_args=" -fno-short-enums -fsigned-char " -Dnewlib-mb=true -Dnewlib-locale-info=true -Dthread-local-storage=false -Dmultilib=false -Dsemihost=false -Dtests=false -Dpicocrt=false -Dpicolib=false -Dposix-console=true -Dprefix=`readlink -f ../../out` -Dspecsdir=`readlink -f ../../out`/picolibc/arm-none-eabi/lib -Doptimization=2
ninja
ninja install
popd

