#!/bin/bash

set -ex

sudo ln -s /usr/local /opt/local
brew update
brew upgrade cmake
brew install jack
brew install dylibbundler
brew install cairo
brew install fontconfig
