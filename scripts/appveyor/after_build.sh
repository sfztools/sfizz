#!/bin/bash
set -ex

make DESTDIR=${PWD}/${INSTALL_DIR} install
tar -zcvf "${INSTALL_DIR}.tar.gz" ${INSTALL_DIR}

# Only release a tarball if there is a tag
if [[ ${APPVEYOR_REPO_TAG} ]]; then
  mv "${INSTALL_DIR}.tar.gz" ${APPVEYOR_BUILD_FOLDER}
fi

cd ${APPVEYOR_BUILD_FOLDER}
