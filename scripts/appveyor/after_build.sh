#!/bin/bash
set -ex

make DESTDIR=${PWD}/${INSTALL_DIR} install

# Set bundle icons
# Note: disabled, rejected by the code-sign step
if false; then
  bundle_icns=/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/KEXT.icns
  for bundle in sfizz.vst3 sfizz.component sfizz.lv2; do
    fileicon set "${INSTALL_DIR}"/"$bundle" "$bundle_icns"
  done
fi

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
fi

# Create the DMG
cat > sfizz-dmg.json << EOF
{
  "title": "sfizz",
  "background": "${APPVEYOR_BUILD_FOLDER}/mac/dmg-back.png",
  "window": {
      "size": { "width": 500, "height": 500 }
  },
  "contents": [
    { "x": 100, "y": 50, "type": "file", "path": "${INSTALL_DIR}/sfizz.vst3" },
    { "x": 250, "y": 50, "type": "file", "path": "${INSTALL_DIR}/sfizz.component" },
    { "x": 400, "y": 50, "type": "file", "path": "${INSTALL_DIR}/sfizz.lv2" },
    { "x": 100, "y": 400, "type": "link", "path": "/Library/Audio/Plug-Ins/VST3" },
    { "x": 250, "y": 400, "type": "link", "path": "/Library/Audio/Plug-Ins/Components" },
    { "x": 400, "y": 400, "type": "link", "path": "/Library/Audio/Plug-Ins/LV2" }
  ]
}
EOF
~/node_modules/appdmg/bin/appdmg.js sfizz-dmg.json "${INSTALL_DIR}.dmg"

# Code-sign the DMG
if test ! -z "${CODESIGN_PASSWORD}"; then
  security unlock-keychain -p dummypasswd build.keychain
  codesign --sign "${CODESIGN_IDENTITY}" --keychain build.keychain --force --verbose "${INSTALL_DIR}.dmg"
fi

# Only release a tarball if there is a tag
if [[ ${APPVEYOR_REPO_TAG} ]]; then
  mv "${INSTALL_DIR}.dmg" ${APPVEYOR_BUILD_FOLDER}
fi

cd ${APPVEYOR_BUILD_FOLDER}
