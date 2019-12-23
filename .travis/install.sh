#!/bin/bash

set -ex

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev lv2-dev

  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 100
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version
fi
