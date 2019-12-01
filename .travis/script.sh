#!/bin/bash

set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 100
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version

  mkdir build && cd build

  # FIXME: lto error in build when enabled
  /usr/local/bin/cmake -D CMAKE_BUILD_TYPE=Release -D ENABLE_LTO=OFF ..
  make -j$(nproc)
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  mkdir build && cd build
  cmake -D SFIZZ_JACK=OFF -G Xcode .. # FIXME: client build
  xcodebuild -project sfizz.xcodeproj -alltargets -configuration Debug build
fi
