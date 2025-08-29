#!/bin/bash
set -e

# Проверка, что сценарий запущен из корня проекта.
if [ ! -d $PWD/.git ]; then
  echo "ERROR. Run script from project root: build/build-linux.sh"
  exit 1
fi

rm -rf bld
mkdir bld
cd bld
cmake --preset=linux-debug ..
cmake --build ./linux-debug
ctest
cmake --install ./linux-debug
cpack -G "ZIP" -B .

