#!/bin/bash

set -ex

sudo ln -s /usr/local /opt/local
brew cask uninstall --force java
brew update
brew upgrade cmake
brew install python ||  brew link --overwrite python
brew install jack
brew install dylibbundler
