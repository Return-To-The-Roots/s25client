#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
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

INSTALL_DIR="${TRAVIS_BUILD_DIR}/installed"
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

# Set runtime path for boost libraries
CMAKE_VARS="$(cmake -LA -N .)"
if echo "${CMAKE_VARS}" | grep -q Boost_LIBRARY_DIR_DEBUG; then
    boostLibDir="$(echo "${CMAKE_VARS}" | grep Boost_LIBRARY_DIR_DEBUG | cut -d "=" -f2)"
else
    boostLibDir="$(echo "${CMAKE_VARS}" | grep Boost_DIR | cut -d "=" -f2)"
    boostLibDir="${boostLibDir%/lib/*}/lib"
fi
# ... but not if it is a system boost because that would fail on OSX due to messing up the search order
if ! [[ "$boostLibDir" =~ ^/usr/ ]]; then
	export DYLD_LIBRARY_PATH="${boostLibDir}${DYLD_LIBRARY_PATH+":$DYLD_LIBRARY_PATH"}"
	export LD_LIBRARY_PATH="${boostLibDir}${LD_LIBRARY_PATH+":$LD_LIBRARY_PATH"}"
fi

# Execute tests
export RTTR_DISABLE_ASSERT_BREAKPOINT=1
export UBSAN_OPTIONS=print_stacktrace=1
export AUDIODEV=null # Avoid errors like: ALSA lib confmisc.c:768:(parse_card) cannot find card '0'
export SDL_VIDEODRIVER=dummy # Avoid crash on travis
if ! ctest --output-on-failure -j2; then
    cat CMakeCache.txt
    echo "LD:${LD_LIBRARY_PATH:-}"
    echo "DYLD:${DYLD_LIBRARY_PATH:-}"
    ctest --output-on-failure --rerun-failed --extra-verbose
fi
