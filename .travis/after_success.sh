#!/bin/bash

set -e

export VERSION=$(git describe --tags)
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  export DEPLOYFILE=Sfizz-$VERSION-x86_64.AppImage
  wget -c -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
  chmod +x linuxdeploy-x86_64.AppImage
  for size in 16 32 48 128 256; do dirname="sfizz/usr/share/icons/hicolor/${size}x${size}/apps"; mkdir -p $dirname; cp ./resources/icons/icon_${size}px.png ./${dirname}/sfizz.png; done
  ./linuxdeploy-x86_64.AppImage --appdir=sfizz --desktop-file=./resources/linux/sfizz.desktop --executable=./build/clients/sfizz_jack --output=appimage
elif [ "$TRAVIS_OS_NAME" = "osx"   ]; then
  export DEPLOYFILE=sfizz-$VERSION.dmg
  mkdir ./output
  cp -r /build/* ./output/
  cp ./resources/icons/icon.icns ./output/sfizz.app/Contents/Resources/
  hdiutil create /tmp/tmp.dmg -ov -volname "sfizz-$VERSION" -fs HFS+ -srcfolder "./output/"
  hdiutil convert /tmp/tmp.dmg -format UDZO -o ./$DEPLOYFILE;
fi
