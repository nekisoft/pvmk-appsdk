#!/usr/bin/env bash
#docs_build.sh
#Makes documentation for PVMK SDK
#Bryan E. Topp <betopp@betopp.com> 2024


mkdir -p out/docs
OUTDIR=$(readlink -f out/docs)

pushd src/docs/intro
#Twice to update refs
pdflatex  -output-directory=${OUTDIR} -halt-on-error intro.latex
pdflatex  -output-directory=${OUTDIR} -halt-on-error intro.latex







