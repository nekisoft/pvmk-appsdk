#!/usr/bin/env bash
#build.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Change to script directory
cd "${0%/*}"

mkdir out
mkdir obj
for BUILDSCRIPT in src/*build.sh
do
	echo "-->" $BUILDSCRIPT
	. $BUILDSCRIPT
done

 
