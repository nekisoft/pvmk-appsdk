#!/usr/bin/env bash
#docs_build.sh
#Makes documentation for PVMK SDK
#Bryan E. Topp <betopp@betopp.com> 2024

set -e

mkdir -p out/docs
TMPDIR=$(mktemp -d)
OUTDIR=$(readlink -f out/docs)

pushd src/docs/intro
#Twice to update refs
pdflatex  -output-directory=${TMPDIR} -halt-on-error intro.latex
pdflatex  -output-directory=${TMPDIR} -halt-on-error intro.latex


cp ${TMPDIR}/*.pdf ${OUTDIR}





