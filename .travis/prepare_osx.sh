#!/bin/bash

set -ex

test -z "$CONTAINER"

if ! [ -z "$CONTAINER" ]; then
  . .travis/docker_container.sh
else
  . .travis/no_container.sh
fi

cd build
buildenv make DESTDIR=${PWD}/${INSTALL_DIR} install

# Bundle LV2 dependencies
cd "${INSTALL_DIR}"/Library/Audio/Plug-Ins/LV2
dylibbundler -od -b -x sfizz.lv2/Contents/Binary/sfizz.so -d sfizz.lv2/Contents/libs/ -p @loader_path/../libs/
dylibbundler -od -b -x sfizz.lv2/Contents/Binary/sfizz_ui.so -d sfizz.lv2/Contents/libs/ -p @loader_path/../libs/
cd "${TRAVIS_BUILD_DIR}/build"

# Bundle VST3 dependencies
cd "${INSTALL_DIR}"/Library/Audio/Plug-Ins/VST3
dylibbundler -od -b -x sfizz.vst3/Contents/MacOS/sfizz -d sfizz.vst3/Contents/libs/ -p @loader_path/../libs/
cd "${TRAVIS_BUILD_DIR}/build"

# Bundle AU dependencies
cd "${INSTALL_DIR}"/Library/Audio/Plug-Ins/Components
dylibbundler -od -b -x sfizz.component/Contents/Resources/plugin.vst3/Contents/MacOS/sfizz -d sfizz.component/Contents/Resources/plugin.vst3/Contents/libs/ -p @loader_path/../libs/
cd "${TRAVIS_BUILD_DIR}/build"

#
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${TRAVIS_TAG} != "" ]]; then
  mv "${INSTALL_DIR}.tar.gz" ${TRAVIS_BUILD_DIR}
fi

cd "${TRAVIS_BUILD_DIR}"
