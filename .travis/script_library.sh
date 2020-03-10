#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_TYPE=Release -DSFIZZ_LV2=OFF -DSFIZZ_VST=OFF -DSFIZZ_TESTS=OFF ..
make -j$(nproc)
