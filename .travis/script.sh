#!/bin/bash

set -ex

mkdir -p build/${INSTALL_DIR} && cd build

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  # FIXME: lto error in build when enabled
  /usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} ..
  make -j$(nproc)
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
  /usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} ..
  make -j$(sysctl -n hw.ncpu)

# Xcode not currently supported, see https://gitlab.kitware.com/cmake/cmake/issues/18088
# xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
fi
