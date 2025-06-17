#!/usr/bin/env sh
#launch_unix_sh.sh
#Launches user's shell with PATH set up for PVMK SDK
#Bryan E. Topp <betopp@betopp.com> 2025

echo Putting the SDK at the end of the PATH...
export PATH=$PATH:$(readlink -f $(dirname $0))/bin
echo

echo SDK built from Git version:
pvmk-sdkversion -vc
echo

echo Welcome to the PVMK SDK for Linux and FreeBSD.
echo Utilities shipped with this SDK all begin with \"pvmk-\".
echo Check out the examples and build them with \"pvmk-make\".
echo Make something fun!

$SHELL

