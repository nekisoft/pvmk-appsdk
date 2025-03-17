#!/usr/bin/env bash
#cmake_build.sh
#Makes CMake for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

#set -x

mkdir -p obj/cmake
pushd obj/cmake

SRCDIR="../../src/cmake-4.0.0-rc2/src"

CXX=clang++
CC=clang

COMMONFLAGS+=" -I${SRCDIR} "
COMMONFLAGS+=" -I${SRCDIR}/cm3p "
COMMONFLAGS+=" -I${SRCDIR}/CursesDialog/form  "
COMMONFLAGS+=" -I${SRCDIR}/cmlibuv/include  "
COMMONFLAGS+=" -I${SRCDIR}/cmcppdap/include  "
COMMONFLAGS+=" -I${SRCDIR}/cmcurl/include "
COMMONFLAGS+=" -I${SRCDIR}/cmcurl/include/curl "
COMMONFLAGS+=" -I/usr/local/include/ "
COMMONFLAGS+=" -g -O2 "
COMMONFLAGS+=" -DKWSYS_NAMESPACE=cmsys -DHAVE_CONFIG_H=1 "

CFLAGS+=${COMMONFLAGS}
CFLAGS+=" -std=c99 -Wall -Wextra -Werror "
#CFLAGS+=" -Wno-clobbered "
CFLAGS+=" -Wno-unused-parameter "
CFLAGS+=" -Wno-null-pointer-subtraction "
#CFLAGS+=" -Wno-dangling-pointer "
CFLAGS+=" -DVI=1 -D_POSIX=1 "

CXXFLAGS+=${COMMONFLAGS}
CXXFLAGS+=" -std=c++14"

LIBS+=" -lncurses -lpthread -lldap -lsysinfo -std=c++14 "

CSRC=`find ${SRCDIR} -name \*.c`
COBJ=
 
CXXSRC=`find ${SRCDIR} -name \*.cxx`
CXXSRC+=" "
CXXSRC+=`find ${SRCDIR} -name \*.cpp`
CXXOBJ=

for CFILE in ${CSRC}
do	
	CFILENAME=$(basename ${CFILE})
	echo ${CFILE}
	OBJFILE=./$(readlink -f ${CFILE}).o
	if [ ! -f ${OBJFILE} ]
	then
		mkdir -p $(dirname ${OBJFILE})
		(set -x; ${CC} ${CFILE} ${CFLAGS} -c -o ${OBJFILE}) || exit -1
	fi
	COBJ+=" "
	COBJ+=${OBJFILE}
done

for CXXFILE in ${CXXSRC}
do
	CXXFILENAME=$(basename ${CXXFILE})
	echo ${CXXFILE}
	OBJFILE=./$(readlink -f ${CXXFILE}).o
	if [ ! -f ${OBJFILE} ]
	then
		mkdir -p $(dirname ${OBJFILE})
		(set -x; ${CXX} ${CXXFILE} ${CXXFLAGS} -c -o ${OBJFILE}) || exit -1
		
	fi
	CXXOBJ+=" "
	CXXOBJ+=${OBJFILE}
done

OUTDIR="../../out"
mkdir -p ${OUTDIR}/bin/
${CXX} ${COBJ} ${CXXOBJ} ${LIBS} -o ${OUTDIR}/bin/pvmk-cmake

popd
