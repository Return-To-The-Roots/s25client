#!/bin/bash

set -euo pipefail

ARCH="${1:?Arch not set. Should be e.g. windows.x86_64}"
CI_TYPE="${2:?Type not set (nightly, stable, other)}"

# Safety check

if [ ! -d ".git" ] || ! git grep -q "Return To The Roots" README.md; then
    echo "Not run in a git repo" >&2
    exit 1
fi

set -x

echo "Git status for main and sub repos:"
git status
git submodule foreach git status

TOOLCHAIN=
if [ "$(uname -s | tr "[:upper:]" "[:lower:]").$(uname -m)" != "${ARCH}" ] ; then
    TOOLCHAIN="$(pwd)/cmake/toolchains/c.${ARCH}.cmake"
fi

BUILD_TYPE=RelWithDebInfo
if [[ "${ARCH}" == apple.* ]]; then
    # Current apple compiler doesn't work with debug info and we can't extract them anyway
    BUILD_TYPE=Release
else
    BUILD_TYPE=RelWithDebInfo
fi

# Auto detect version and revision
VERSION_FLAGS="-DRTTR_VERSION=OFF -DRTTR_REVISION=OFF"

if [ "${CI_TYPE}" == "nightly" ] ; then
  MAKE_TARGET=create_nightly
elif [ "${CI_TYPE}" == "stable" ] ; then
  MAKE_TARGET=create_stable
  VERSION_FLAGS="-DRTTR_VERSION=$(cat .stable-version) -DRTTR_REVISION=OFF"
else
  MAKE_TARGET=install
fi

CMAKE_VERSION="3.8.2"
CMAKE_DIR="$(pwd)/installedCMake-${CMAKE_VERSION}"
tools/ci/installCMake.sh "${CMAKE_VERSION}" "${CMAKE_DIR}" cmakeSrc
export PATH="${CMAKE_DIR}/bin:${PATH}"

INSTALL_DIR="$(pwd)/installed"
rm -rf "${INSTALL_DIR}"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    ${VERSION_FLAGS} \
    -DRTTR_ENABLE_WERROR=ON \
    -DRTTR_USE_STATIC_BOOST=ON \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
make ${MAKE_TARGET}
