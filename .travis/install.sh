#!/bin/bash

set -ex
. .travis/environment.sh

if [[ ${CROSS_COMPILE} == "mingw32" || ${CROSS_COMPILE} == "mingw64" ]]; then
  buildenv pacman -Sqy --noconfirm
  buildenv pacman -Sq --noconfirm base-devel wget mingw-w64-cmake mingw-w64-gcc mingw-w64-pkg-config mingw-w64-libsndfile
  buildenv i686-w64-mingw32-gcc -v && buildenv i686-w64-mingw32-g++ -v && buildenv i686-w64-mingw32-cmake --version
elif [[ ${BUILD_TYPE} == "lv2" ]]; then
  wget -q https://github.com/sfztools/sndfile-libraries/releases/download/${TRAVIS_OS_NAME}/sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}.tar.gz
  tar xf sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}.tar.gz
  sudo cp -R sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}/usr /
elif [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version
fi
