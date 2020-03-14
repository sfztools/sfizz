#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_VST=ON \
      -DSFIZZ_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j$(sysctl -n hw.ncpu)
# Xcode not currently supported, see https://gitlab.kitware.com/cmake/cmake/issues/18088
# xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
