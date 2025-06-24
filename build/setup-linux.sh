#!/bin/bash
set -e

projectDir="$PWD"
if [ ! -d "$projectDir/.git" ]; then
  echo "ERROR! Run this script from the project root: build/$(basename $0)"
  exit 1
fi

cd $HOME
if [ -d "$HOME/vcpkg" ]; then
  rm -rf vcpkg
fi

# Install vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
mv vcpkg ~/bin/vcpkg
export VCPKG_ROOT=$HOME/vcpkg

# Install external dependencies
vcpkg install             \
    gtest                 \
    nlohmann-json         \
    vulkan-sdk-components \
    qt5                   \
    qt5-wayland           \
    opencascade
