#!/bin/bash

set -ex

sudo ln -s /usr/local /opt/local
brew update
brew upgrade python ||  brew link --overwrite python
brew upgrade cmake
brew install jack
brew install dylibbundler
