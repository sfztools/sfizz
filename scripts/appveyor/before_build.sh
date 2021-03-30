#!/bin/bash
set -ex

git submodule update --init --recursive
mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -DSFIZZ_VST=ON \
      -DSFIZZ_AU=ON \
      -DSFIZZ_RENDER=OFF \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_TESTS=ON \
      -DCMAKE_CXX_STANDARD=14 \
      -DLV2PLUGIN_INSTALL_DIR=/ \
      -DVSTPLUGIN_INSTALL_DIR=/ \
      -DAUPLUGIN_INSTALL_DIR=/ \
      ..
