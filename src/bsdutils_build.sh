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
CFLAGS+=" -std=c11 -Werror "
CFLAGS+=" -Wno-clobbered "
CFLAGS+=" -D_DEFAULT_SOURCE"

OUTDIR=../../out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

SHARED=${SRCDIR}/shared/*.c



#Windows needs libs linked for regex
LIBS+=$(pkg-config regex --cflags --libs || true)

#Linux doesn't have a separate libiconv... check what the iconv util links against
if [[ $(ldd $(which iconv) | grep iconv) ]]
then
	LIBS+=" -liconv"
fi


${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/rm/*.c       -o ${PLATDIR}/pvmk-rm       -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/realpath/*.c -o ${PLATDIR}/pvmk-realpath -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/mkdir/*.c    -o ${PLATDIR}/pvmk-mkdir    -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/cp/*.c       -o ${PLATDIR}/pvmk-cp       -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/date/*.c     -o ${PLATDIR}/pvmk-date     -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/cat/*.c      -o ${PLATDIR}/pvmk-cat      -static ${LIBS}
${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/find/*.c     -o ${PLATDIR}/pvmk-find     -static ${LIBS}


popd
