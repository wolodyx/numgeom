#!/bin/bash
set -e

apt update && apt upgrade -y
apt -y install git cmake curl wget python3 gnupg2 vim build-essential bison flex ninja-build
apt -y install libboost-all-dev
apt -y install qtbase5-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxcb-cursor0
apt -y install libwayland-dev
apt -y install '~n^libocct-.*-dev$' libtbb-dev

# Install vulkan-sdk
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc \
 && wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
apt update && apt -y install vulkan-sdk

# Install linuxdeploy
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT
cd ${TEMP_DIR}
wget -O linuxdeploy https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
chmod +x ./linuxdeploy
./linuxdeploy --appimage-extract
cp -r ./squashfs-root/usr/* /usr/local/
cp -r ./squashfs-root/plugins /usr/
rm -rf ./squashfs-root ./linuxdeploy
apt-get -y install file fuse3

reboot
