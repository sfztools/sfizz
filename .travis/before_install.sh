#!/bin/bash

set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  wget https://cmake.org/files/v3.13/cmake-3.13.0-Linux-x86_64.sh
  sudo sh cmake-3.13.0-Linux-x86_64.sh --skip-license --prefix=/usr/local
  export PATH="/usr/local/bin:$PATH" # FIXME
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  sudo ln -s /usr/local /opt/local
  brew update
  brew upgrade cmake
  brew install jack
fi
