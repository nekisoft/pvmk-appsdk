#!/usr/bin/env bash
#clean.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Change to script directory
cd "${0%/*}"

#Make sure we're in the right directory before rm'ing a bunch of shit
if [[ ! -f clean.sh ]] ; then
    echo no clean.sh in script directory - aborting
    exit 1
fi

if [[ ! -f build.sh ]] ; then
    echo no build.sh in script directory - aborting
    exit 1
fi

rm -rf out
rm -rf obj

