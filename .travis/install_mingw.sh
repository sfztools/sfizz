#!/bin/bash

set -ex
. .travis/docker_container.sh

buildenv_as_root pacman -Sqyu --noconfirm
buildenv_as_root pacman -Sq --noconfirm base-devel wget mingw-w64-cmake mingw-w64-gcc mingw-w64-pkg-config mingw-w64-libsndfile mingw-w64-cairo
buildenv i686-w64-mingw32-gcc -v && buildenv i686-w64-mingw32-g++ -v && buildenv i686-w64-mingw32-cmake --version
