#!/bin/bash

set -euo pipefail

CMAKE_VERSION="${1:?Missing CMake version}"
INSTALL_DIR="${2:?Missing install dir}"
CONFIGURE_PREFIX_DIR="${3:?Missing directory prefix where to build CMake}" # Where to store downloaded and build files
DOWNLOAD_ONLY="${4:-no}"

export PATH="${INSTALL_DIR}/bin:${PATH}"

curVersion="$(cmake --version 2>/dev/null || true)"

if echo "${curVersion}" | grep -q "cmake version ${CMAKE_VERSION}"; then
    echo "CMake ${CMAKE_VERSION} already installed: `which cmake`"
    exit 0
fi

mkdir -p "${CONFIGURE_PREFIX_DIR}" && cd "${CONFIGURE_PREFIX_DIR}"

# Encode version into directory to find it again
BUILD_DIR="cmake-${CMAKE_VERSION}"
if [ ! -f "${BUILD_DIR}/bootstrap" ]; then
    CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz"
    wget "${CMAKE_URL}" -qO- | tar xz
    if [ ! -f "${BUILD_DIR}/bootstrap" ]; then
        echo "Download failed or files invalid" >&2
        exit 1
    fi
fi

if [ "${DOWNLOAD_ONLY}" != "no" ]; then
    exit 0
fi

cd "${BUILD_DIR}"

# Linux and OSX version
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu)

# Build quietely preferably with cmake (if it exists) or fallback to bootstrap
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" >/dev/null || \
    ./bootstrap --prefix="${INSTALL_DIR}" --parallel=${NPROC} >/dev/null

make install -j${NPROC} >/dev/null
