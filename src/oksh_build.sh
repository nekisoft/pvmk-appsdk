#!/usr/bin/env bash
#oksh_build.sh
#Makes OpenBSD Korn Shell for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -x
set -e

mkdir -p obj/oksh
pushd obj/oksh

SRCDIR="../../src/oksh-7.6/src"

CC=gcc

CFLAGS+=" -I${SRCDIR}"
CFLAGS+=" -g -O2"
CFLAGS+=" -std=c99 -Wall -Wextra -Werror "
CFLAGS+=" -Wno-clobbered "
CFLAGS+=" -DVI=1 -D_POSIX=1 "

CSRC=`find ${SRCDIR} -name \*.c`
COBJ=
for CFILE in ${CSRC}
do	
	CFILENAME=$(basename ${CFILE})
	echo ${CFILENAME}
	OBJFILE=./$(readlink -f ${CFILE}).o
	if [ ! -f ${OBJFILE} ]
	then
		mkdir -p $(dirname ${OBJFILE})
		${CC} ${CFILE} ${CFLAGS} -c -o ${OBJFILE} -static || exit -1
	fi
	COBJ+=" "
	COBJ+=${OBJFILE}
done

OUTDIR="../../out"
mkdir -p ${OUTDIR}/bin/
mkdir -p ${OUTDIR}/bin/$(uname -o)/$(uname -m)
${CC} ${COBJ} -o ${OUTDIR}/bin/$(uname -o)/$(uname -m)/pvmk-oksh -static

popd
