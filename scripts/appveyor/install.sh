#!/bin/bash

set -e
# do not `set -x` until after processing the secrets

# -- secret --
echo "* Set up code-signing"

if test -z "${CODESIGN_PASSWORD}"; then
  echo "! Secrets not available, skip code-signing"
else
  echo "1. Extract PKCS12"
  echo "${CODESIGN_P12}" | base64 -D -o codesign.p12
  stat codesign.p12
  echo "2. Extract CA certificate"
  echo "${SFZTOOLS_CRT}" | base64 -D -o sfztools.crt
  stat sfztools.crt

  echo "3. Create a new keychain"
  security create-keychain -p dummypasswd build.keychain
  echo "4. Configure the new keychain as default"
  security default-keychain -s build.keychain
  echo "5. Unlock the new keychain"
  security unlock-keychain -p dummypasswd build.keychain

  echo "6. Import the code-signing key pair"
  security import codesign.p12 -k build.keychain -P "${CODESIGN_PASSWORD}" -T /usr/bin/codesign
  rm -f codesign.p12
  echo "7. Import the trusted CA certificate"
  sudo security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain sfztools.crt
  rm -f sfztools.crt

  echo "8. Set up the code-signing ACL"
  security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k dummypasswd build.keychain

  echo "9. Check the code-signing identity"
  security find-identity -p codesigning build.keychain
fi
# -- /secret --

set -x

brew install libsndfile dylibbundler

cd ~; npm install appdmg; cd -
~/node_modules/appdmg/bin/appdmg.js --version
