#!/bin/bash

set -ex
. .travis/environment.sh

cd build
buildenv make install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}
mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
cd ..
