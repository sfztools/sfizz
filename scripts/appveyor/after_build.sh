#!/bin/bash
set -ex

make DESTDIR=${PWD}/${INSTALL_DIR} install

# Set bundle icons
bundle_icns=/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/KEXT.icns
for bundle in sfizz.vst3 sfizz.component sfizz.lv2; do
  fileicon set "${INSTALL_DIR}"/"$bundle" "$bundle_icns"
done

# Perform code-signing
if test -z "${CODESIGN_PASSWORD}"; then
  echo "! Secrets not available, skip code-signing"
else
  # unlock the keychain
  security unlock-keychain -p dummypasswd build.keychain
  # code-sign VST3 and dylibs
  codesign --sign "${CODESIGN_IDENTITY}" --deep --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/sfizz.vst3
  # code-sign AudioUnit and dylibs
  codesign --sign "${CODESIGN_IDENTITY}" --deep --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/sfizz.component
  # code-sign LV2 and dylibs (note: manual, LV2 are not real bundles)
  codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
           "${INSTALL_DIR}"/sfizz.lv2/Contents/Binary/*.so
  if ls "${INSTALL_DIR}"/sfizz.lv2/Contents/Frameworks/*.dylib &> /dev/null; then
    codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose \
             "${INSTALL_DIR}"/sfizz.lv2/Contents/Frameworks/*.dylib
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

# Need the flag --skip-jenkins to prevent CI hanging
# https://github.com/create-dmg/create-dmg/issues/72
create-dmg --skip-jenkins "${INSTALL_DIR}.dmg" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${APPVEYOR_REPO_TAG} ]]; then
  mv "${INSTALL_DIR}.dmg" ${APPVEYOR_BUILD_FOLDER}
fi

cd ${APPVEYOR_BUILD_FOLDER}
