#!/usr/bin/env bash
#nemul_build.sh
#Builds Neki32 simulator for SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e
set -x


mkdir -p obj
cd obj

cp -r ../src/nemul ./nemul
cd nemul

OUTDIR=$(readlink -f ../../out)
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi

${MAKE}

cp bin/nemul.* ${PLATDIR}/pvmk-nemul

