#!/usr/bin/env bash
#oksh_build.sh
#Makes OpenBSD Korn Shell for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -x

mkdir -p obj/make
pushd obj/make

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
		${CC} ${CFILE} ${CFLAGS} -c -o ${OBJFILE} || exit -1
	fi
	COBJ+=" "
	COBJ+=${OBJFILE}
done

OUTDIR="../../out"
mkdir -p ${OUTDIR}/bin/
${CC} ${COBJ} -o ${OUTDIR}/bin/pvmk-oksh

popd
