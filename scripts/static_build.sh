#!/bin/bash

# Works for ubuntu 18.04 in a container...

sed -Ei 's/^# deb-src /deb-src /' /etc/apt/sources.list
apt-get update
apt-get install -y build-essential cmake wget pkg-config automake gettext libtool-bin git
apt-get source libogg-dev libvorbis-dev libflac-dev libsndfile

prefix=$(pwd)/prefix
flags="-fPIC -I${prefix}/include"

mkdir -p $prefix
pushd libogg-1.3.2
CFLAGS=$flags CXXFLAGS=$flags PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig ./configure --disable-shared --prefix=$prefix
make -j$(nproc)
make install
popd

pushd libvorbis-1.3.5
./autogen.sh
CFLAGS=$flags CXXFLAGS=$flags PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig ./configure --disable-shared --prefix=$prefix
make -j$(nproc)
make install
popd

pushd flac-1.3.2
./autogen.sh
CFLAGS=$flags CXXFLAGS=$flags PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig ./configure --disable-shared --prefix=$prefix
make -j$(nproc)
make install
popd

pushd libsndfile-1.0.28
CFLAGS=$flags CXXFLAGS=$flags PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig ./configure --disable-shared --disable-full-suite --prefix=$prefix
make -j$(nproc)
make install
popd

PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig cmake .. -DSFIZZ_STATIC_LIBSNDFILE=ON -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF -DCMAKE_BUILD_TYPE=Release
make -j 16
strip -s sfizz.lv2/sfizz.so
