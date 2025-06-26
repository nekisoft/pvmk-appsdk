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
pdflatex  -output-directory=${TMPDIR} -interaction=batchmode intro.latex || true
pdflatex  -output-directory=${TMPDIR} -interaction=batchmode intro.latex || true
cp ${TMPDIR}/*.pdf ${OUTDIR} || true
popd

pushd src/docs
cp ARM_DDI01001.pdf ${OUTDIR}
popd

