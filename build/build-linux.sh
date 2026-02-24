#!/bin/bash
set -e

PRESET="linux-vcpkg-debug"

# Проверка, что сценарий запущен из корня проекта.
if [ ! -d $PWD/.git ]; then
  echo "ERROR. Run script from project root: build/build-linux.sh"
  exit 1
fi

rm -rf bld
mkdir bld
cd bld
cmake --preset=${PRESET} ..
cmake --build ./${PRESET}
ctest --test-dir ./${PRESET}
cmake --install ./${PRESET}
cd ${PRESET}
cpack -G "ZIP" -B .
