#!/usr/bin/env bash
#make_build.sh
#Makes GNU Make for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -x

mkdir -p obj/make
pushd obj/make

SRCDIR="../../src/make-4.4.1/src"

CC=gcc

CFLAGS+=" -I${SRCDIR}"
CFLAGS+=" -g -O2"
CFLAGS+=" -std=c99 -Wall -Wextra -Werror "
CFLAGS+=" -Wno-unused-parameter "
CFLAGS+=" -Wno-unused-but-set-variable "
CFLAGS+=" -Wno-unused-variable "
CFLAGS+=" -DHAVE_CONFIG_H=1 "

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
${CC} ${COBJ} -o ${OUTDIR}/bin/pvmk-make

popd
