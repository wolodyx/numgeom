#!/bin/bash
set -e

NUGET_FEED_URL="https://192.168.0.13/v3/index.json"
NUGET_API_KEY="123456"

projectDir="$PWD"
if [ ! -d "$projectDir/.git" ]; then
  echo "ERROR! Run this script from the project root: build/nuget-server/$(basename $0)"
  exit 1
fi

# Download `vcpkg`` executable file!
if [ ! -f ./vcpkg/vcpkg ]; then
  git submodule update --init --recursive --reference vcpkg
  cd vcpkg
  ./bootstrap-vcpkg.sh -disableMetrics
  cd ..
fi

# Download and get `nuget.exe` full path!
NUGET_EXECUTABLE=$(./vcpkg/vcpkg fetch nuget | tail -n 1)

# Add nuget sources.
mono "${NUGET_EXECUTABLE}" sources add -Source ${NUGET_FEED_URL} -Name vcpkg

# Set api key for package pushing
mono "${NUGET_EXECUTABLE}" \
  setapikey ${NUGET_API_KEY} \
  -Source ${NUGET_FEED_URL}

