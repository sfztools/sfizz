#!/bin/bash
# Adapted from Distrho's package-osx-bundles.sh script

set -e

if [ -d build ]; then
  cd build
else
  echo "Please run this script from the root folder"
  exit
fi

rm -rf au
rm -rf lv2
rm -rf vst3

mkdir au lv2 vst3
mv sfizz.component au/
mv sfizz.lv2 lv2/
mv sfizz.vst3 vst3/

pkgbuild \
  --identifier "sfz.tools.sfizz.au.bundle" \
  --install-location "/Library/Audio/Plug-Ins/Components/" \
  --root "${PWD}/au/" \
  sfz-tools-sfizz-au-bundle.pkg

pkgbuild \
  --identifier "sfz.tools.sfizz.lv2.bundle" \
  --install-location "/Library/Audio/Plug-Ins/LV2/" \
  --root "${PWD}/lv2/" \
  sfz-tools-sfizz-lv2-bundle.pkg

pkgbuild \
  --identifier "sfz.tools.sfizz.vst3.bundle" \
  --install-location "/Library/Audio/Plug-Ins/VST3/" \
  --root "${PWD}/vst3/" \
  sfz-tools-sfizz-vst3-bundle.pkg

cd ..
