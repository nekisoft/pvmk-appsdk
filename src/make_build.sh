#!/usr/bin/env bash
#make_build.sh
#builds GNU Make for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out/bin/$(uname -o)/$(uname -m)/

mkdir -p obj
cd obj
tar -xf ../src/make-4.4.1.tar.gz

mkdir make-build
cd make-build

OUTDIR=$(readlink -f ../..)/out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)/

../make-4.4.1/configure CFLAGS="-std=c99 -D_cdecl=__cdecl" MAKEINFO="true" LDFLAGS="--static" --prefix=${OUTDIR} --disable-nls --program-prefix=pvmk- --bindir=${PLATDIR} --libdir=${PLATDIR} --libexecdir=${PLATDIR} --includedir=${OUTDIR}/trash --mandir=${OUTDIR}/trash --infodir=${OUTDIR}/trash --docdir=${OUTDIR}/trash --without-guile 

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi

${MAKE}
${MAKE} install

