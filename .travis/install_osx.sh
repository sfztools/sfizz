#!/bin/bash

set -ex

sudo ln -s /usr/local /opt/local
brew update

for brew_package in cmake jack cairo fontconfig dylibbundler; do
  brew install "$brew_package" || brew upgrade "$brew_package"
done
