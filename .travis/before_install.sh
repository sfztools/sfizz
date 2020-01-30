#!/bin/bash

set -ex

cmake_dir="cmake-3.13.0-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}"
cmake_arc="${cmake_dir}.tar.gz"
cmake_url="https://github.com/sfztools/cmake/releases/download/${TRAVIS_OS_NAME}/${cmake_arc}"

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  wget -q ${cmake_url}
  tar xzf ${cmake_arc}
  cd ${TRAVIS_BUILD_DIR}/${cmake_dir}
  sudo cp    bin/*   /usr/local/bin/
  sudo cp -r share/* /usr/local/share/
  export PATH="/usr/local/bin:$PATH" # FIXME
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
  sudo ln -s /usr/local /opt/local
  brew update
  brew upgrade cmake
  brew install jack
fi
