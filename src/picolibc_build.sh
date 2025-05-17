#!/usr/bin/env bash
#picolibc_build.sh
#Builds picolibc for PVMK app SDK

set -e
set -x

mkdir -p obj
mkdir -p out
pushd obj
tar -xf ../src/picolibc-1.8.6.tar.xz
popd

mkdir -p obj/picolibc-build
pushd obj/picolibc-build

OUTDIR=$(readlink -f ../../out)

../picolibc-1.8.6/scripts/do-arm-configure -Dc_args=" -fno-short-enums -fsigned-char " -Dnewlib-mb=true -Dnewlib-locale-info=true -Dthread-local-storage=false -Dmultilib=false -Dsemihost=false -Dtests=false -Dpicocrt=false -Dpicolib=false -Dposix-console=true -Dprefix=${OUTDIR} -Dspecsdir=${OUTDIR}/armv5te-pvmk-eabi/lib -Doptimization=2 -Dincludedir=${OUTDIR}/armv5te-pvmk-eabi/include -Dlibdir=${OUTDIR}/armv5te-pvmk-eabi/lib
ninja
ninja install
popd

