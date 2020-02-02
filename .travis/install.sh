#!/bin/bash

set -ex
. .travis/environment.sh

if [[ ${CROSS_COMPILE} == "mingw32" || ${CROSS_COMPILE} == "mingw64" ]]; then
  buildenv pacman -Sqy --noconfirm
  buildenv pacman -Sq --noconfirm base-devel wget mingw-w64-cmake mingw-w64-gcc mingw-w64-pkg-config mingw-w64-libsndfile
  buildenv i686-w64-mingw32-gcc -v && buildenv i686-w64-mingw32-g++ -v && buildenv i686-w64-mingw32-cmake --version
elif [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev lv2-dev
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version
fi
