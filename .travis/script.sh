#!/bin/bash

set -ex
. .travis/environment.sh

mkdir -p build/${INSTALL_DIR} && cd build

if [[ ${CROSS_COMPILE} == "mingw32" ]]; then

  buildenv i686-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF ..
  buildenv make -j
elif [[ ${CROSS_COMPILE} == "mingw64" ]]; then

  buildenv x86_64-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF ..
  buildenv make -j
elif [[ ${BUILD_TYPE} == "lv2" ]]; then

  buildenv cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_TESTS=OFF -DSFIZZ_SHARED=OFF -DSFIZZ_STATIC_LIBSNDFILE=ON ..
  buildenv make -j
elif [[ ${TRAVIS_OS_NAME} == "linux" ]]; then

  buildenv cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_LV2=OFF -DSFIZZ_TESTS=OFF ..
  buildenv make -j$(nproc)
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

  buildenv cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_TESTS=OFF ..
  buildenv make -j$(sysctl -n hw.ncpu)

# Xcode not currently supported, see https://gitlab.kitware.com/cmake/cmake/issues/18088
# xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
fi
