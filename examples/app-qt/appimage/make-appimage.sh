#!/bin/bash

set -e

source_dir=$1

if [ -d ./AppDir ]; then
  rm -rf ./AppDir
fi
cmake --install . --prefix=./AppDir/usr
export LD_LIBRARY_PATH=./AppDir/usr/lib
linuxdeploy --appdir AppDir
cp ${source_dir}/appimage/app.desktop ./AppDir/usr/share/applications/app.desktop
cp ${source_dir}/appimage/app-icon-128x128.png ./AppDir/usr/share/icons/hicolor/128x128/apps/AppIcon.png
linuxdeploy --appdir AppDir --output appimage
