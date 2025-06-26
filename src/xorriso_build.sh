#!/usr/bin/env bash
#xorriso_build.sh
#Builds xorriso for PVMK app SDK

set -e
set -x

mkdir obj/xorriso-build
pushd obj/xorriso-build
THISDIR=$(readlink -f .)
../../out/bin/pvmk-make -C../../src/xorriso-1.5.6/ BINDIR=${THISDIR}/bin OBJDIR=${THISDIR}/obj
rm -f ../../out/bin/pvmk-xorriso
mkdir -p ../../out/bin/$(uname -o)/$(uname -m)
cp ${THISDIR}/bin/pvmk-xorriso ../../out/bin/$(uname -o)/$(uname -m)/pvmk-xorriso
popd

