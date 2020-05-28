#!/bin/bash

set -ex

sudo ln -s /usr/local /opt/local
brew update
brew upgrade cmake
brew install jack cairo fontconfig dylibbundler
