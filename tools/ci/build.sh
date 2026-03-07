#!/bin/bash

# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

# Information
cmake --version
$CXX --version

if [[ "${BUILD_TYPE}" == "Release" ]]; then
    RTTR_BUNDLE="ON"
    MAKE_TARGET="install"
else
    RTTR_BUNDLE="OFF"
    MAKE_TARGET="all"
fi

INSTALL_DIR="${GITHUB_WORKSPACE}/installed"
rm -rf "${INSTALL_DIR}"
mkdir -p build && cd build
if ! cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
        -DRTTR_ENABLE_WERROR=ON \
        -DRTTR_EDITOR_ADMINMODE=ON \
        -DRTTR_BUNDLE="${RTTR_BUNDLE}" \
        -G "Unix Makefiles" ${ADDITIONAL_CMAKE_FLAGS}; then
    cat CMakeFiles/CMakeOutput.log
    cat CMakeFiles/CMakeError.log
    exit 1
fi

make -j3 ${MAKE_TARGET} || make VERBOSE=1 ${MAKE_TARGET}
