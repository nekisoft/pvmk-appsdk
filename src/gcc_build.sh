#!/usr/bin/env bash
#gcc_build.sh
#builds GNU GCC for PVMK application SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out
mkdir -p obj
cd obj
tar -xf ../src/gcc-15.1.0.tar.xz

mkdir gcc-build
cd gcc-build

#Dumb patch - gcc for some reason tries to link the PIC version of libiberty, no idea why
#So patch the Makefile, make it always decide not to
patch ../gcc-15.1.0/gcc/Makefile.in <<EOF
1128c1128
< LIBIBERTY_PICDIR=\$(if \$(findstring mingw,\$(target)),,pic)
---
> LIBIBERTY_PICDIR=
EOF

patch ../gcc-15.1.0/c++tools/Makefile.in <<EOF
93,95d92
< ifneq (\$(PICFLAG),)
< override LIBIBERTY := ../libiberty/pic/libiberty.a
< endif
EOF


OUTDIR=$(readlink -f ../../out)
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

../gcc-15.1.0/configure MAKEINFO="true" LDFLAGS="--static" --prefix=${OUTDIR} --target=armv5te-pvmk-eabi --with-native-system-header-dir=${OUTDIR}/armv5te-pvmk-eabi/include --disable-shared --disable-multiarch --disable-multilib --with-endian=little --disable-threads --disable-tls --with-cpu=arm926ej-s --with-mode=arm --disable-libssp --disable-nls --program-prefix=pvmk- --bindir=${PLATDIR} --libdir=${PLATDIR} --libexecdir=${PLATDIR} --enable-languages=c,c++ --disable-hosted-libstdcxx --without-headers --disable-host-shared --infodir=${OUTDIR}/trash --mandir=${OUTDIR}/trash --docdir=${OUTDIR}/trash --enable-host-static --enable-static --disable-shared --disable-lto --disable-host-pie 

mkdir -p  ${OUTDIR}/armv5te-pvmk-eabi/include

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi

${MAKE}
${MAKE} install

mv ../../out/armv5te-pvmk-eabi/bin/* ../../out/bin/$(uname -o)/$(uname -m)/

pushd ../../out/bin/$(uname -o)/$(uname -m)/gcc/armv5te-pvmk-eabi/15.1.0
mv include include-gcc
popd

rm -rf ../../out/armv5te-pvmk-eabi/bin

