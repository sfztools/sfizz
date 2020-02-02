#!/bin/bash

set -ex

if [[ ${CROSS_COMPILE} == "mingw32" ]]; then
  pacman -Sqy --noconfirm
  pacman -Sq --noconfirm base-devel wget mingw-w64-cmake mingw-w64-gcc mingw-w64-pkg-config mingw-w64-libsndfile
  i686-w64-mingw32-gcc -v && i686-w64-mingw32-g++ -v && cmake --version && $SHELL --version
elif [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev lv2-dev
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version
fi
