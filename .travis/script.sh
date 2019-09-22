#!/bin/bash

set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 100
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version

  mkdir build && cd build
  /usr/local/bin/cmake -D CMAKE_BUILD_TYPE=Release -D SFIZZ_CLIENTS=ON ..
  make -j$(nproc)
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  mkdir build && cd build
  cmake -D SFIZZ_CLIENTS=OFF -G Xcode .. # FIXME: client build
  xcodebuild -project sfizz.xcodeproj -alltargets -configuration Release build
fi
