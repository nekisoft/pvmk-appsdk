#!/usr/bin/env bash
#build.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Change to script directory
cd "${0%/*}"

mkdir out
mkdir obj

. ./src/picolibc_build.sh
. ./src/libsc_build.sh
. ./src/pvmkoslib_build.sh
. ./src/pvmkpicocrt_build.sh
. ./src/savepart_build.sh
. ./src/wrappers_build.sh
. ./src/examples_build.sh
 
