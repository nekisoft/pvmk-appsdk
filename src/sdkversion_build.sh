#!/usr/bin/env bash
#sdkversion_build.sh
#Makes SDK version printer for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -x
set -e

mkdir -p obj/sdkversion
pushd obj/sdkversion

SRCDIR="../../src/sdkversion"

CC=gcc

CFLAGS+=" -I${SRCDIR}"
CFLAGS+=" -g -O2"
CFLAGS+=" -std=c99 -Werror -Wall -Wextra -pedantic"
CFLAGS+=" -D_POSIX=1 "

BUILDVERSION=$(git describe --abbrev=8 --dirty --always --tags)
BUILDDATE=$(date -u +%FT%TZ)
BUILDUSER=$(whoami)@$(hostname)
BUILDHOST=$(uname -o)
BUILDMACHINE=$(uname -m)
BUILDCHANGE=$(git log -n 1 --date=iso-strict --pretty=format:%cI)

CFLAGS+=" "
CFLAGS+=-DBUILDVERSION=
CFLAGS+=\"${BUILDVERSION}\"
CFLAGS+=" "
CFLAGS+=-DBUILDDATE=
CFLAGS+=\"${BUILDDATE}\"
CFLAGS+=" "
CFLAGS+=-DBUILDUSER=
CFLAGS+=\"${BUILDUSER}\"
CFLAGS+=" "
CFLAGS+=-DBUILDCHANGE=
CFLAGS+=\"${BUILDCHANGE}\"
CFLAGS+=" "
CFLAGS+=-DBUILDHOST=
CFLAGS+=\"${BUILDHOST}\"
CFLAGS+=" "
CFLAGS+=-DBUILDMACHINE=
CFLAGS+=\"${BUILDMACHINE}\"
CFLAGS+=" "

OUTDIR=../../out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

${CC} ${CFLAGS} ${SHARED} ${SRCDIR}/*.c       -o ${PLATDIR}/pvmk-sdkversion       -static ${LIBS}


popd
