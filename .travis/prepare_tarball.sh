#!/bin/bash

set -ex

if ! [ -z "$CONTAINER" ]; then
  . .travis/docker_container.sh
else
  . .travis/no_container.sh
fi

# Do not prepare a tarball without a tag
if [[ ${TRAVIS_TAG} == "" ]]; then
  exit 0
fi

cd build
buildenv make DESTDIR=${PWD}/${INSTALL_DIR} install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}
mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
cd ..
