#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_VST=ON \
      -DSFIZZ_AU=ON \
      -DSFIZZ_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=14 \
      -DLV2PLUGIN_INSTALL_DIR=/Library/Audio/Plug-Ins/LV2 \
      -DVSTPLUGIN_INSTALL_DIR=/Library/Audio/Plug-Ins/VST3 \
      -DAUPLUGIN_INSTALL_DIR=/Library/Audio/Plug-Ins/Components \
      ..
make -j$(sysctl -n hw.ncpu)
# Xcode not currently supported, see https://gitlab.kitware.com/cmake/cmake/issues/18088
# xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
