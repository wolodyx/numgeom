#!/bin/bash
set -e

apt update
apt -y install           \
    git                  \
    cmake                \
    build-essential      \
    ninja-build          \
    flex                 \
    bison                \
    curl                 \
    zip                  \
    unzip                \
    tar                  \
    file                 \
    pkg-config           \
    autoconf             \
    autoconf-archive     \
    automake             \
    libtool              \
    python3-venv         \
    python3-pip          \
    wayland-protocols    \
    libwayland-dev       \
    libwayland-client0   \
    libxtst-dev          \
    libxinerama-dev      \
    libxcursor-dev       \
    '^libxcb.*-dev'      \
    libx11-xcb-dev       \
    libgl1-mesa-dev      \
    libxrender-dev       \
    libxi-dev            \
    libxkbcommon-dev     \
    libxkbcommon-x11-dev \
    libsystemd-dev       \
    libarchive-dev       \
    libxrandr-dev

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
mkdir -p $HOME/bin
mv vcpkg $HOME/bin/vcpkg
export PATH=$PATH:$HOME/bin
export VCPKG_ROOT=$HOME/vcpkg

# Install external dependencies
vcpkg install             \
    gtest                 \
    nlohmann-json         \
    boost-log             \
    vulkan-loader[xcb]    \
    vulkan-sdk-components \
    opencascade           \
    qt5-base[vulkan]

rm -rf \
    $VCPKG_ROOT/buildtrees \
    $VCPKG_ROOT/downloads
