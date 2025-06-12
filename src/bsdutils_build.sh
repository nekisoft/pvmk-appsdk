#!/usr/bin/env bash
#bsdutils_build.sh
#Makes subset of BSD Utilities for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -x
set -e

mkdir -p obj/bsdutils
pushd obj/bsdutils

SRCDIR="../../src/bsdutils"

CC=gcc

CFLAGS+=" -I${SRCDIR}"
CFLAGS+=" -g -O2"
CFLAGS+=" -std=c99 -Werror "
CFLAGS+=" -Wno-clobbered "
CFLAGS+=" -D_POSIX=1 "

OUTDIR=../../out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

SHARED=${SRCDIR}/shared/*.c

${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/date/*.c -o ${PLATDIR}/pvmk-date -static
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/cat/*.c -o ${PLATDIR}/pvmk-cat -static
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/find/*.c -o ${PLATDIR}/pvmk-find -static

popd
