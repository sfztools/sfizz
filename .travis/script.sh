#!/bin/bash

set -ex

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  mkdir -p build/${INSTALL_DIR} && cd build

  # FIXME: lto error in build when enabled
  /usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF -DCMAKE_INSTALL_PREFIX:PATH=${PWD}/${INSTALL_DIR} ..
  make -j$(nproc)
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
  mkdir build && cd build
  cmake -D SFIZZ_JACK=OFF -G Xcode .. # FIXME: client build
  xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
fi
