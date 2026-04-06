#!/bin/bash

set -e

TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT
cd ${TEMP_DIR}

wget -O linuxdeploy https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
chmod +x ./linuxdeploy
./linuxdeploy --appimage-extract
cp -r ./squashfs-root/usr/* /usr/local/
cp -r ./squashfs-root/plugins /usr/
rm -rf ./squashfs-root ./linuxdeploy

apt-get -y install file fuse
