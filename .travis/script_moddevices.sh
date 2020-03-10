#!/bin/bash

set -ex
. .travis/docker_container.sh

mkdir -p build/${INSTALL_DIR} && cd build

buildenv mod-plugin-builder /usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF ..
buildenv mod-plugin-builder make -j
