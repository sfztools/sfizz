#!/bin/bash

set -ex

cmake_dir="cmake-3.13.0-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}"
cmake_arc="${cmake_dir}.tar.gz"
cmake_url="https://github.com/sfztools/cmake/releases/download/${TRAVIS_OS_NAME}/${cmake_arc}"

wget -q ${cmake_url}
tar xzf ${cmake_arc}
cd ${TRAVIS_BUILD_DIR}/${cmake_dir}
sudo cp    bin/*   /usr/local/bin/
sudo cp -r share/* /usr/local/share/
