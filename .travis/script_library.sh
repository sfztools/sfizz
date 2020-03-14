#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_LV2=OFF \
      -DSFIZZ_VST=OFF \
      -DSFIZZ_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j$(nproc)
