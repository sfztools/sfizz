#!/bin/bash

set -ex

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  cd build
  make install
  tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}
  mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
  cd ..
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
  echo "TODO: xcodebuild make install or dmg image?"
fi
