#!/usr/bin/env bash
#xorriso_build.sh
#Builds xorriso for PVMK app SDK

mkdir obj/xorriso-build
pushd obj/xorriso-build
THISDIR=$(readlink -f .)
../../out/bin/pvmk-make -C../../src/xorriso-1.5.6/ BINDIR=${THISDIR}/bin OBJDIR=${THISDIR}/obj
rm -f ../../out/bin/pvmk-xorriso
cp ${THISDIR}/bin/pvmk-xorriso ../../out/bin/
popd

