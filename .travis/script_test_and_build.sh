#!/bin/bash
set -ex

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_JACK=ON \
      -DSFIZZ_VST=ON \
      -DSFIZZ_LV2_UI=ON \
      -DSFIZZ_TESTS=ON \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_STATIC_DEPENDENCIES=OFF \
      -DSFIZZ_LV2=ON \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j2 sfizz_tests
tests/sfizz_tests
make -j2 sfizz_jack
make -j2 sfizz_render
make -j2 sfizz_lv2
make -j2 sfizz_lv2_ui
make -j2 sfizz_vst3
