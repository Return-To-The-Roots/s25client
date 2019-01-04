#!/bin/bash

set -euo pipefail

VERSION="${1:?Missing Boost version}"
INSTALL_DIR="${2:?Missing install dir}"

required_libs=( filesystem system program_options thread test locale iostreams regex )

all_libs_exist() {
    for lib in "${required_libs[@]}"; do
        if ! ls "${INSTALL_DIR}/lib/*${lib}*.*" &> /dev/null; then
            echo "Missing Boost.${lib}. Building boost..."
            return 1
        fi
    done
    return 0
}

if all_libs_exist; then
    exit 0
fi

BUILD_DIR=/tmp/boost

mkdir -p "${BUILD_DIR}" && cd "${BUILD_DIR}"

FILE_NAME="boost_${VERSION//./_}"
URL="https://sourceforge.net/projects/boost/files/boost/${VERSION}/${FILE_NAME}.tar.bz2/download"
wget "${URL}" -qO- | tar jx
if [ ! -f "${FILE_NAME}/bootstrap.sh" ]; then
    echo "Download failed or files invalid" >&2
    exit 1
fi

cd "${FILE_NAME}"

# Linux and OSX version
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu)

./bootstrap.sh --with-libraries=filesystem,system,program_options,thread,test,locale,iostreams,regex threading=multi >/dev/null
./b2 link=shared variant=release --prefix="${INSTALL_DIR}" -j${NPROC} install >/dev/null
