#!/usr/bin/env bash
#build.sh for python3 port to neki32/pvmk
#Bryan E. Topp <betopp@betopp.com> 2024

set -x

#Directory containing toolchain
TOOLBIN=./out/bin/

#Toolchain parts
CC=${TOOLBIN}/pvmk-cc

#Directory containing Python source files
SRCDIR=./src/cpython-3.13.1/

#Object directory for intermediates
OBJDIR=./obj/cpython

#Build list of all source files
CFILES=$(find ${SRCDIR} -name \*.c)

#Flags used when compiling
CFLAGS+="-I${SRCDIR}/Include "
CFLAGS+="-I${SRCDIR}/Include/internal "
CFLAGS+="-std=c11 "
CFLAGS+="-DPy_BUILD_CORE=1 "

#Compile each source file into a corresponding object
for CFILE in ${CFILES}
do
	#echo ${CFILE}
	OFILE=${OBJDIR}/${CFILE}.o
	OFILES+=" "
	OFILES+=${OFILE}
	mkdir -p $(dirname ${OFILE})
	#echo $OFILE
	if [ ! -f ${OFILE} ]
	then
		${CC} ${CFLAGS} -c ${CFILE} -o ${OFILE} || exit -1
	fi
done

#Link em
${CC} ${OFILES} -o ./cpython.nne

