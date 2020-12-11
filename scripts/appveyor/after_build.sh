#!/bin/bash
set -ex

make DESTDIR=${PWD}/${INSTALL_DIR} install

# Perform code-signing
if test -z "${CODESIGN_PASSWORD}"; then
  echo "! Secrets not available, skip code-signing"
else
  # unlock the keychain
  security unlock-keychain -p dummypasswd build.keychain
  # code-sign VST3 and dylibs
  codesign --sign "${CODESIGN_IDENTITY}" --deep --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/Library/Audio/Plug-Ins/VST3/sfizz.vst3
  # code-sign AudioUnit and dylibs
  codesign --sign "${CODESIGN_IDENTITY}" --deep --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/Library/Audio/Plug-Ins/Components/sfizz.component
  # code-sign LV2 and dylibs (note: manual, LV2 are not real bundles)
  codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/Library/Audio/Plug-Ins/LV2/sfizz.lv2/Contents/Binary/*.so
  if ls "${INSTALL_DIR}"/Library/Audio/Plug-Ins/LV2/sfizz.lv2/Contents/Frameworks/*.dylib &> /dev/null; then
    codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
             "${INSTALL_DIR}"/Library/Audio/Plug-Ins/LV2/sfizz.lv2/Contents/Frameworks/*.dylib
  fi
  if ls "${INSTALL_DIR}"/usr/local/bin/* &> /dev/null; then
    codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
      "${INSTALL_DIR}"/usr/local/bin/*
  fi
  if ls "${INSTALL_DIR}"/usr/local/lib/*.dylib &> /dev/null; then
    codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
      "${INSTALL_DIR}"/usr/local/lib/*.dylib
  fi
fi

tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${APPVEYOR_REPO_TAG} ]]; then
  mv "${INSTALL_DIR}.tar.gz" ${APPVEYOR_BUILD_FOLDER}
fi

cd ${APPVEYOR_BUILD_FOLDER}
