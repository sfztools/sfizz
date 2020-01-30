#!/bin/bash

set -ex

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev lv2-dev
  gcc -v && g++ -v && cmake --version && /usr/local/bin/cmake --version && $SHELL --version
fi
