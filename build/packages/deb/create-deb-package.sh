#!/bin/bash

install_dir=$1

mkdir -p package/opt/numgeom/{bin,lib}
cp -r ${install_dir}/bin package/opt/numgeom
cp -r ${install_dir}/lib package/opt/numgeom
dpkg-deb --build ./package
