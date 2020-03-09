#!/bin/bash
set -ex

wget -q https://github.com/sfztools/sndfile-libraries/releases/download/${TRAVIS_OS_NAME}/sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}.tar.gz
tar xf sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}.tar.gz
sudo cp -R sndfile-libraries-${TRAVIS_OS_NAME}-${TRAVIS_CPU_ARCH}/usr /
