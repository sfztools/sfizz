#!/bin/bash

set -ex

if ! [ -z "$CONTAINER" ]; then
  . .travis/docker_container.sh
else
  . .travis/no_container.sh
fi

cd build
buildenv make DESTDIR=${PWD}/${INSTALL_DIR} install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${TRAVIS_TAG} != "" ]]; then
  mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
fi

cd ..
