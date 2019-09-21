#!/bin/bash

set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  sudo apt-get install libasound2-dev libjack-jackd2-dev libsndfile1-dev
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  brew install libsndfile
fi
