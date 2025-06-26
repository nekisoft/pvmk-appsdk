#!/usr/bin/env bash
#wx_build.sh
#Builds minimal wxWidgets for shipping static nemul build
#Bryan E. Topp <betopp@betopp.com> 2025

set -e
set -x

#Extract source tarball to a working directory under obj
mkdir -p obj/wx
cd obj
cd wx
tar -xf ../../src/wxWidgets-3.2.8.tar.bz2

#Make a place to "make install"
mkdir -p pfx

#Build it...


cd wxWidgets-3.2.8
./configure CXXFLAGS=" -std=c++20 " --prefix=$(readlink -f ../pfx)  --disable-plugins --disable-tests --enable-universal --disable-shared --disable-unicode --disable-intl --disable-webview  --without-libjpeg --without-libpng --without-regex --without-zlib --without-expat --without-libtiff --disable-compat30 --without-gtk --disable-svg --without-themes --without-opengl --disable-stc --disable-sockets --disable-webrequest --disable-secretstore --without-cairo --with-x11

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi

${MAKE} -j
${MAKE} install

