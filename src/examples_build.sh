#!/usr/bin/env bash
#examples_build.sh
#Copies examples for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Follow all symlinks in the copy.
#This blows up the size of the output a bit, but makes it far far more stable on Windows environments
mkdir out/examples
cp -rL src/examples/* out/examples/

