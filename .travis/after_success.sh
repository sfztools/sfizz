#!/bin/bash

set -ex

cd build
make install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}
mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
cd ..
