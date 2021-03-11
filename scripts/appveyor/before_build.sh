#!/bin/bash
set -ex

git submodule update --init --recursive
mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_VST=ON \
      -DSFIZZ_AU=ON \
      -DSFIZZ_RENDER=OFF \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_TESTS=ON \
      -DCMAKE_CXX_STANDARD=14 \
      -DBUILD_SHARED_LIBS=OFF \
      -DLV2PLUGIN_INSTALL_DIR=/ \
      -DVSTPLUGIN_INSTALL_DIR=/ \
      -DAUPLUGIN_INSTALL_DIR=/ \
      ..
