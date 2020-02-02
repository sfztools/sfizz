#!/bin/bash

set -ex
. .travis/environment.sh

mkdir -p build/${INSTALL_DIR} && cd build

if [[ ${CROSS_COMPILE} == "mingw32" ]]; then

  buildenv i686-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} -DSFIZZ_JACK=OFF ..
  buildenv make -j
elif [[ ${CROSS_COMPILE} == "mingw64" ]]; then

  buildenv x86_64-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} -DSFIZZ_JACK=OFF ..
  buildenv make -j
elif [[ ${TRAVIS_OS_NAME} == "linux" ]]; then

  buildenv cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} ..
  buildenv make -j$(nproc)
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

  buildenv cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} ..
  buildenv make -j$(sysctl -n hw.ncpu)

# Xcode not currently supported, see https://gitlab.kitware.com/cmake/cmake/issues/18088
# xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
fi
