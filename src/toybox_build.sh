#!/usr/bin/env bash
#toybox_build.sh
#builds Toybox utils for PVMK application SDK (need this stuff on Windows!)
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out/bin/$(uname -o)/$(uname -m)/

mkdir -p obj
cd obj
tar -xf ../src/toybox-0.8.12.tar.gz
cd toybox-0.8.12
cp ../../src/toybox.config ./.config

#Toybox scripts call out /bin/bash on their bangpath - use /usr/bin/env bash instead
find . -name "*.sh" | xargs -n1 sed -i -e 's/\/bin\/bash/\/usr\/bin\/env bash/g'

#Toybox always wants to build "taskset" and "ps" command...?
echo "" > toys/other/taskset.c
echo "" > toys/posix/ps.c

#Patch out xattr support in cp
patch toys/posix/cp.c <<EOF
123a124
> #if 0
147a149
> #endif
174a177
> #if 0
175a179,180
> #endif
> 
301c306
< 
---
> #if 0
302a308
> #endif
EOF

#Patch out xattr support in tar
patch toys/posix/tar.c <<EOF
392a393
> #if 0
420a422
> #endif
EOF

OUTDIR=$(readlink -f ../..)/out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)/

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi
LDFLAGS="--static" ${MAKE} toybox

mkdir -p ${PLATDIR}
cp toybox ${PLATDIR}/toybox
