#!/bin/bash

set -ex
. .travis/docker_container.sh

mkdir -p build/${INSTALL_DIR} && cd build
if [[ ${CROSS_COMPILE} == "mingw32" ]]; then
  buildenv i686-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_STATIC_LIBSNDFILE=ON ..
  buildenv make -j
elif [[ ${CROSS_COMPILE} == "mingw64" ]]; then
  buildenv x86_64-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_STATIC_LIBSNDFILE=ON ..
  buildenv make -j
fi
