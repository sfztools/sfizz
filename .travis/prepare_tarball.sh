#!/bin/bash

set -ex

cd build
make DESTDIR=${PWD}/${INSTALL_DIR} install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${TRAVIS_TAG} != "" ]] && [[ ${DEPLOY_BUILD} ]]; then
  mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
fi

cd ..
